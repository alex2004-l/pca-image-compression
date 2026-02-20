#ifndef __PCA_LIB_HPP
#define __PCA_LIB_HPP

#include <stdint.h>
#include <string>

extern uint32_t num_threads;

/*
 * Function for compressing a PCA image
 * @param path           path to save the compressed image
 * @param img            image buffer
 * @param rows, cols     image dimensions
 * @param nof_components used by PCA decomposition
 */
void compress_pca(const std::string &path, unsigned char *img,
                uint32_t rows, uint32_t cols, int nof_components);

/*
 * Function for decompressing a PCA image
 * @param path  path to the compressed image
 * 
 * @return      a buffer to the image data
 */
unsigned char* decompress_pca(const std::string path);

#endif /* __PCA_LIB_HPP */
