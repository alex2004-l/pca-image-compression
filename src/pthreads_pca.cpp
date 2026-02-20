#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <numeric>
#include <stdint.h>
#include <vector>
#include <pthread.h>

#include <Eigen/Dense>

#include "pca_lib.hpp"
#include "compressed_pca.hpp"
#include "threadpool.hpp"
#include "pthreads_funcs.hpp"


double *convert_uchar_to_double(const unsigned char *arr, int size, ThreadPool *tp)
{
    double *new_arr = new double[size];
    threadpool_launch(arr, new_arr, size, 1, tp, process_convert_uchar_to_double);
    return new_arr;
}


unsigned char *convert_double_to_uchar(const double *arr, int size, ThreadPool *tp)
{
    unsigned char *new_arr = new unsigned char[size];
    threadpool_launch(arr, new_arr, size, 1, tp, process_convert_double_to_uchar);
    return new_arr;
}


double compute_mean_matrix(double *data, uint32_t rows, uint32_t columns, ThreadPool *tp)
{
    double mean = 0.0f;
    double *means = new double[num_threads - 1];
    threadpool_launch(data, means, rows, columns, tp, process_compute_mean_matrix);
    for (uint32_t i = 0; i < num_threads - 1; i++)
        mean += means[i];
    delete[] means;
    return mean / (rows * columns);
}


double normalize_matrix(double *data, uint32_t rows, uint32_t cols, ThreadPool *tp)
{
    double mean_val = compute_mean_matrix(data, rows, cols, tp);
    threadpool_launch(&mean_val, data, rows, cols, tp, process_normalize_matrix);
    return mean_val;
}


double *transpose_matrix(const double *data, uint32_t rows, uint32_t cols, ThreadPool *tp)
{
    double *transposed = new double[rows * cols];
    threadpool_launch(data, transposed, rows, cols, tp, process_transpose_matrix);
    return transposed;
}


double *multiply_matrices(const double *mat1, const double *mat2,
                        uint32_t m, uint32_t n, uint32_t p, ThreadPool *tp)
{
    double *result = new double[m * p];
    std::fill(result, result + (m * p), 0.0);

    pthread_arg *args = new struct pthread_arg[num_threads - 1];
    sem_t finished;
    sem_init(&finished, PTHREAD_PROCESS_PRIVATE, 0);

    for (uint32_t i = 0; i < num_threads - 1; i++) {
        args[i].id       = i;
        args[i].data     = mat1;
        args[i].sec_data = mat2;
        args[i].out      = result;
        args[i].rows     = m;
        args[i].cols     = n;
        args[i].cols2    = p;
        args[i].finished = &finished;

        struct thread_pool_arg task;
        task.arg = (void*)(&(args[i]));
        task.function = process_multiply_matrices;
        tp->enqueue_task(task);
    }

    for (uint32_t i = 0; i < num_threads - 1; i++) 
        sem_wait(&finished); 

    sem_destroy(&finished);

    delete[] args;
    return result;
}


double *compute_covariance_matrix(double *img, uint32_t rows, uint32_t cols, ThreadPool *tp)
{
    double *transposed = transpose_matrix(img, rows, cols, tp);
    double *result     = multiply_matrices(transposed, img, cols, rows, cols, tp);
    double factor      = 1.0 / (rows - 1);
    threadpool_launch(&factor, result, rows, cols, tp, process_compute_covariance_matrix);
    delete[] transposed;
    return result;
}


double compute_dot_product(uint32_t size, double *a, double *b, ThreadPool *tp)
{
    double result = 0.0f;
    double *results = new double[num_threads - 1];
    threadpool_launch(a, results, size, 1, tp, process_compute_dot_product, b);
    for (uint32_t i = 0; i < num_threads - 1; i++)
        result += results[i];
    delete[] results;
    return result;
}


double compute_norm(double *vec, uint32_t size, ThreadPool *tp)
{
    return std::sqrt(compute_dot_product(size, vec, vec, tp));
}


double* compute_eigen_vectors(double *data, int dim)
{
    Eigen::Map<Eigen::MatrixXd> cov(data, dim, dim);
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> es(cov);
  
    Eigen::MatrixXd Q            = es.eigenvectors();
    double* eigenvectors_ptr     = new double[dim * dim];

    Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> Qr = Q;
    memcpy(eigenvectors_ptr, Qr.data(), dim * dim * sizeof(double));
    
    return eigenvectors_ptr;
}


double *lanczos_algorithm(const double *data, uint32_t size, ThreadPool *tp, int steps = 0)
{
    steps = (steps <= 0)? size : steps;

    double *v      = new double[size];
    double *v_prev = new double[size];

    std::fill(v, v + size, std::sqrt(1.0/size));

    double *alpha = new double[steps];
    double  *beta = new double[steps];

    double* V = new double[steps * size];

    double* proj = new double[steps + 1];
    std::fill(proj, proj + steps + 1, 0.0);

    for (int i = 0; i < steps; i++) {
        double *w = multiply_matrices(data, v, size, size, 1, tp);
        memcpy(V + i * size, v, size * sizeof(double));
        
        if (i > 0) {
            for (uint32_t j = 0; j < size; j++) {
                w[j] -= beta[i - 1] * v_prev[j];
            }
        }

        alpha[i] = compute_dot_product(size, v, w, tp);

        threadpool_launch(v, w, size, 1, tp, process_lanczos_algorithm_1, &(alpha[i]));

        // The overhead of parallelizing this part with
        // OMP is way bigger and causes a slow-down
        for (int t = 0; t <= i; t++) {
            double* v_t = V + t * size;
            proj[t] = compute_dot_product(size, v_t, w, tp);
        }

        for (int t = 0; t <= i; t++) {
            double p_val = proj[t];

            for (uint32_t j = 0; j < size; j++)
                w[j] -= p_val * V[t * size + j];
        }

        beta[i] = compute_norm(w, size, tp);

        std::swap(v, v_prev);

        threadpool_launch(w, v, size, 1, tp, process_lanczos_algorithm_2, &(beta[i]));
        delete[] w;
    }

    double *T = new double[size * size];
    std::fill(T, T + (size * size), 0.0);

    for (int i = 0; i < steps; i++) {
        T[i * steps + i] = alpha[i];

        if (i < steps - 1) {
            T[i * steps + (i+1)] = beta[i];
            T[(i+1) * steps + i] = beta[i];
        }
    }

    double *V_transposed = transpose_matrix(V, steps, size, tp);
    double *eigen_vals   = compute_eigen_vectors(T, steps);
    double *result       = multiply_matrices(V_transposed, eigen_vals, size, steps, steps, tp);

    delete[] v;
    delete[] V;
    delete[] T;
    delete[] v_prev;
    delete[] V_transposed;
    delete[] alpha;
    delete[] beta;
    delete[] eigen_vals;
    delete[] proj;

    return result;
}


void compress_pca(const std::string &path, unsigned char *img,
                uint32_t rows, uint32_t cols, int nof_components)
{
    ThreadPool *tp = new ThreadPool(num_threads - 1);
    double *double_img = convert_uchar_to_double(img, rows * cols, tp);
    double  mean_val   = normalize_matrix(double_img, rows, cols, tp);  // Normalize data
    double *cov_mat    = compute_covariance_matrix(double_img, rows, cols, tp);
    // Get top nof_components eigenvectors using Lanczos algorithm
    double *eigenvectors_ptr = nullptr;
    
    try {
        eigenvectors_ptr = lanczos_algorithm(cov_mat, cols, tp, nof_components);
    } catch (std::exception &e) {
        std::cerr << "Caught exception " << e.what() << "\n";
        exit(EXIT_FAILURE);
    }

    double *projected_image = multiply_matrices(double_img, eigenvectors_ptr, rows, cols, nof_components, tp);
    save_compressed_pca(path, projected_image, eigenvectors_ptr, mean_val, rows, cols, nof_components);

    delete tp;
    delete[] double_img;
    delete[] cov_mat;
    delete[] eigenvectors_ptr;
    delete[] projected_image;
}


unsigned char* decompress_pca(const std::string path)
{
    uint32_t rows, cols;
    int nof_components;

    ThreadPool *tp = new ThreadPool(num_threads - 1);
    double *projected_matrix = nullptr;
    double *eigenvectors     = nullptr;
    double  mean_val         = 0.0f;

    load_compressed_pca(path, &projected_matrix, &eigenvectors,
                    mean_val,rows, cols, nof_components);

    double *transposed_eigenvectors =
                    transpose_matrix(eigenvectors, cols, nof_components, tp);
    double *reconstructed_image = multiply_matrices(projected_matrix,
                    transposed_eigenvectors, rows, nof_components, cols, tp);

    // Add back mean
    threadpool_launch(&mean_val, reconstructed_image, rows, cols, tp, process_decompress_pca);

    unsigned char *result = convert_double_to_uchar(reconstructed_image, rows * cols, tp);

    delete tp;
    delete[] projected_matrix;
    delete[] eigenvectors;
    delete[] transposed_eigenvectors;
    delete[] reconstructed_image;

    return result;
}
