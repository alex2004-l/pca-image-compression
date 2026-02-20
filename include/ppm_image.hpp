#ifndef __PPM_IMAGE_HPP
#define __PPM_IMAGE_HPP

#include <string>
#include <stdint.h>

#define RGB_IMAGE        "P6"
#define GRAYSCALE_IMAGE  "P5"

constexpr double RED_COEF   = 0.299f;
constexpr double BLUE_COEF  = 0.114f;
constexpr double GREEN_COEF = 0.587f;

constexpr int RGB_NOF_CHANNELS       = 3;
constexpr int GRAYSCALE_NOF_CHANNELS = 1;

/*
 * Struct for representing a PPM image
 */
struct ppm_image {
    std::string   format;
    uint32_t  width;
    uint32_t  height;
    uint32_t  max_val;
    uint32_t  nof_channels;
    unsigned char *data;
};

/*
 * Function for loading a PPM image from a file
 * @param file_path   path to the PPM image file
 * 
 * @return            pointer to the loaded ppm_image struct
 */
ppm_image *load_ppm_image(const std::string &file_path);

/*
 * Function for converting an RGB image to grayscale
 * @param original_image   pointer to the original RGB ppm_image struct
 * 
 * @return                 pointer to the new grayscale ppm_image struct
 */
ppm_image *convert_rgb_to_grayscale(ppm_image *original_image);

/*
 * Function for converting a grayscale image to RGB
 * @param original_image   pointer to the original grayscale ppm_image struct
 * 
 * @return                 pointer to the new RGB ppm_image struct
 */
ppm_image *convert_grayscale_to_rgb(ppm_image *original_image);

/*
 * Function for writing a ppm_image struct to a file
 * @param file_path   path to the output PPM image file
 * @param image       pointer to the ppm_image struct to write
 */
void       write_image(const std::string &file_path, ppm_image *image);

#endif /* __PPM_IMAGE_HPP */
