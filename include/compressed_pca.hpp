#ifndef __COMPRESS_PCA_HPP
#define __COMPRESS_PCA_HPP

#include <string>
#include <stdint.h>

/*
 * Function for saving a compressed PCA image to a file
 * @param path              path to save the compressed image
 * @param projected_matrix  pointer to the projected matrix data
 * @param eigenvectors      pointer to the eigenvectors data   
 * @param mean_val          mean value used for normalization
 * @param rows, cols        image dimensions
 * @param num_components    number of components used by PCA decomposition
 */
void save_compressed_pca(const std::string &path, double *projected_matrix, double *eigenvectors,
    double mean_val, uint32_t rows, uint32_t cols, int num_components);

/*
 * Function for loading a compressed PCA image from a file
 * @param path              path to the compressed image
 * @param projected_matrix  pointer to the projected matrix data
 * @param eigenvectors      pointer to the eigenvectors data   
 * @param mean_val          mean value used for normalization
 * @param rows, cols        image dimensions
 * @param num_components    number of components used by PCA decomposition
 */
void load_compressed_pca(const std::string &path, double **projected_matrix, double **eigenvectors,
    double &mean_val, uint32_t &rows, uint32_t &cols, int &num_components);

#endif /* __COMPRESS_PCA_HPP */
