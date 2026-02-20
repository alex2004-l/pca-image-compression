#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <omp.h>
#include <numeric>
#include <stdint.h>
#include <vector>

#include <Eigen/Dense>

#include "pca_lib.hpp"
#include "compressed_pca.hpp"


double *convert_uchar_to_double(const unsigned char *arr, int size)
{
    double *new_arr = new double[size];

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < size; i++) {
        new_arr[i] = static_cast<double>(arr[i]);
    }

    return new_arr;
}


unsigned char *convert_double_to_uchar(const double *arr, int size)
{
    unsigned char *new_arr = new unsigned char[size];

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < size; i++) {
        new_arr[i] = (arr[i] < 0) ? 0 : static_cast<unsigned char>(arr[i]);
    }

    return new_arr;
}


double compute_mean_matrix(double *data, uint32_t rows, uint32_t columns)
{
    double mean = 0.0f;

    #pragma omp parallel for reduction(+:mean)
    for (uint32_t i = 0; i < rows * columns; i++)
        mean += data[i];

    return mean / (rows * columns);
}


double normalize_matrix(double *data, uint32_t rows, uint32_t cols)
{
    double mean_val = compute_mean_matrix(data, rows, cols);

    #pragma omp parallel for schedule(auto)
    for (uint32_t i = 0; i < rows * cols; i++) {
        data[i] -= mean_val;
    }

    return mean_val;
}


double *transpose_matrix(const double *data, uint32_t rows, uint32_t cols)
{
    double *transposed = new double[rows * cols];

    #pragma omp parallel for collapse(2)
    for (uint32_t i = 0; i < rows; i++)
        for (uint32_t j = 0; j < cols; j++) 
            transposed[j * rows + i] = data[i * cols + j];
    
    return transposed;
}


double *multiply_matrices(const double *mat1, const double *mat2,
                        uint32_t m, uint32_t n, uint32_t p)
{
    double *result = new double[m * p];
    std::fill(result, result + (m * p), 0.0);

    #pragma omp parallel for collapse(2) schedule(static)
    for (uint32_t i = 0; i < m; i++) {
        for (uint32_t k = 0; k < n; k++) {
            double temp = mat1[i * n + k];
            for (uint32_t j = 0; j < p; j++) {
                result[i * p + j] += temp * mat2[k * p + j];
            }
        }
    }

    return result;
}


double *compute_covariance_matrix(double *img, uint32_t rows, uint32_t cols)
{
    double *transposed = transpose_matrix(img, rows, cols);
    double *result     = multiply_matrices(transposed, img, cols, rows, cols);
    double factor      = 1.0 / (rows - 1);

    #pragma omp parallel for
    for (uint32_t i = 0; i < cols * cols; i++) {
        result[i] = factor * result[i];
    }

    delete[] transposed;

    return result;
}


double compute_dot_product(uint32_t size, double *a, double *b)
{
    double result = 0.0f;

    #pragma omp parallel for reduction(+:result)
    for (uint32_t i = 0; i < size; i++) {
        result += a[i] * b[i];
    }

    return result;
}


double compute_norm(double *vec, uint32_t size)
{
    return std::sqrt(compute_dot_product(size, vec, vec));
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


double *lanczos_algorithm(const double *data, uint32_t size, int steps = 0)
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
        double *w = multiply_matrices(data, v, size, size, 1);
        memcpy(V + i * size, v, size * sizeof(double));
        
        if (i > 0) {
            for (uint32_t j = 0; j < size; j++) {
                w[j] -= beta[i - 1] * v_prev[j];
            }
        }

        alpha[i] = compute_dot_product(size, v, w);

        #pragma omp parallel for schedule(static)
        for (uint32_t j = 0; j < size; j++) {
            w[j] -= alpha[i] * v[j];
        }

        // The overhead of parallelizing this part with
        // OMP is way bigger and causes a slow-down
        for (int t = 0; t <= i; t++) {
            double* v_t = V + t * size;
            proj[t] = compute_dot_product(size, v_t, w);
        }

        for (int t = 0; t <= i; t++) {
            double p_val = proj[t];

            #pragma omp parallel for schedule(static)
            for (uint32_t j = 0; j < size; j++)
                w[j] -= p_val * V[t * size + j];
        }

        beta[i] = compute_norm(w, size);

        std::swap(v, v_prev);

        #pragma omp parallel for schedule(static)
        for (uint32_t j = 0; j < size; j++)
            v[j] = w[j] / beta[i];
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

    double *V_transposed = transpose_matrix(V, steps, size);
    double *eigen_vals   = compute_eigen_vectors(T, steps);
    double *result       = multiply_matrices(V_transposed, eigen_vals, size, steps, steps);

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
    omp_set_nested(1);
    omp_set_num_threads((int)num_threads);

    double *double_img = convert_uchar_to_double(img, rows * cols);
    double  mean_val   = normalize_matrix(double_img, rows, cols);  // Normalize data
    double *cov_mat    = compute_covariance_matrix(double_img, rows, cols);
    
    // Get top nof_components eigenvectors using Lanczos algorithm
    double *eigenvectors_ptr = nullptr;
    
    try {
        eigenvectors_ptr = lanczos_algorithm(cov_mat, cols, nof_components);
    } catch (std::exception &e) {
        std::cerr << "Caught exception " << e.what() << "\n";
        exit(EXIT_FAILURE);
    }

    double *projected_image = multiply_matrices(double_img, eigenvectors_ptr, rows, cols, nof_components);
    save_compressed_pca(path, projected_image, eigenvectors_ptr, mean_val, rows, cols, nof_components);

    delete[] double_img;
    delete[] cov_mat;
    delete[] eigenvectors_ptr;
    delete[] projected_image;
}


unsigned char* decompress_pca(const std::string path)
{
    uint32_t rows, cols;
    int nof_components;

    double *projected_matrix = nullptr;
    double *eigenvectors     = nullptr;
    double  mean_val         = 0.0f;

    load_compressed_pca(path, &projected_matrix, &eigenvectors,
                    mean_val,rows, cols, nof_components);

    double *transposed_eigenvectors =
                    transpose_matrix(eigenvectors, cols, nof_components);
    double *reconstructed_image = multiply_matrices(projected_matrix,
                    transposed_eigenvectors, rows, nof_components, cols);

    // Add back mean
    #pragma omp parallel for schedule(auto)
    for (uint32_t i = 0; i < rows * cols; i++) {
        reconstructed_image[i] += mean_val;
    }

    unsigned char *result = convert_double_to_uchar(reconstructed_image, rows * cols);

    delete[] projected_matrix;
    delete[] eigenvectors;
    delete[] transposed_eigenvectors;
    delete[] reconstructed_image;

    return result;
}
