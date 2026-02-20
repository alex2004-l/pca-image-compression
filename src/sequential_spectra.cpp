#include <fstream>
#include <iostream>
#include <stdint.h>
#include <stdexcept>

#include <Eigen/Dense>
#include <Spectra/SymEigsSolver.h>
#include <Spectra/MatOp/DenseSymMatProd.h>

#include "pca_lib.hpp"
#include "compressed_pca.hpp"


double *convert_uchar_to_double(const unsigned char *arr, int size)
{
    double *new_arr = new double[size];

    for (int i = 0; i < size; i++) {
        new_arr[i] = static_cast<double>(arr[i]);
    }

    return new_arr;
}


unsigned char *convert_double_to_uchar(const double *arr, int size)
{
    unsigned char *new_arr = new unsigned char[size];

    for (int i = 0; i < size; i++) {
        new_arr[i] = (arr[i] < 0) ? 0 : static_cast<unsigned char>(arr[i]);
    }

    return new_arr;
}


double compute_mean_matrix(double *data, uint32_t rows, uint32_t columns)
{
    double mean = 0.0f;

    for (uint32_t i = 0; i < rows; i++)
        for (uint32_t j = 0; j < columns; j++)
            mean += data[i * columns + j];

    return mean / (rows * columns);
}


double normalize_matrix(double *data, uint32_t rows, uint32_t cols)
{
    double mean_val = compute_mean_matrix(data, rows, cols);

    for (uint32_t i = 0; i < rows; i++) {
        for (uint32_t j = 0; j < cols; j++) {
            data[i * rows + j] -= mean_val;
        }
    }

    return mean_val;
}


double *transpose_matrix(const double *data, uint32_t rows, uint32_t cols)
{
    double *transposed = new double[rows * cols];

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

    for (uint32_t i = 0; i < cols * cols; i++) {
        result[i] = factor * result[i];
    }

    delete[] transposed;

    return result;
}


double* compute_top_eigen_vectors(double *data, int dim, int k)
{
    Eigen::Map<Eigen::MatrixXd> cov(data, dim, dim);
    Spectra::DenseSymMatProd<double> op(cov);

    int m = std::min(dim, k * 2 + 4);

    Spectra::SymEigsSolver<Spectra::DenseSymMatProd<double>> eigs(op, k, m);
    eigs.init();
    eigs.compute(Spectra::SortRule::LargestAlge);

    Eigen::MatrixXd Q        = eigs.eigenvectors();
    double* eigenvectors_ptr = new double[dim * k];

    Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> Qr = Q;
    memcpy(eigenvectors_ptr, Qr.data(), dim * k * sizeof(double));
    
    return eigenvectors_ptr;
}


void compress_pca(const std::string &path, unsigned char *img,
                uint32_t rows, uint32_t cols, int nof_components)
{
    double *double_img = convert_uchar_to_double(img, rows * cols);
    double  mean_val   = normalize_matrix(double_img, rows, cols);
    double *cov_mat    = compute_covariance_matrix(double_img, rows, cols);

    double *eigenvectors_ptr = nullptr;

    try{
        eigenvectors_ptr = compute_top_eigen_vectors(cov_mat, cols, nof_components);
    } catch (std::exception &e) {
        std::cerr << "Caught exception " << e.what() << "\n";
        exit(EXIT_FAILURE);
    }

    double *projected_image  = multiply_matrices(double_img, eigenvectors_ptr, rows, cols, nof_components);

    save_compressed_pca(path, projected_image, eigenvectors_ptr, mean_val, rows, cols, nof_components);

    delete[] double_img;
    delete[] cov_mat;
    delete[] eigenvectors_ptr;
    delete[] projected_image;
}


unsigned char* decompress_pca(const std::string path) {
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
    for (uint32_t i = 0; i < rows; i++) {
        for (uint32_t j = 0; j < cols; j++) {
            reconstructed_image[i * rows + j] += mean_val;
        }
    }

    unsigned char *result = convert_double_to_uchar(reconstructed_image, rows * cols);

    delete[] projected_matrix;
    delete[] eigenvectors;
    delete[] transposed_eigenvectors;
    delete[] reconstructed_image;

    return result;
}
