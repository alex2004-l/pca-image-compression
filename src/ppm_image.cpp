#include <iostream>
#include <fstream>
#include <vector>

#include "ppm_image.hpp"


ppm_image *load_ppm_image(const std::string &file_path)
{
    ppm_image *new_image = new ppm_image;

    std::ifstream in(file_path, std::ios::binary);

    if (!in.is_open()) {
        std::cerr << "Could not open file " << file_path << std::endl;
        exit(EXIT_FAILURE);
    }

    in >> new_image->format;
    in >> new_image->width >> new_image->height >> new_image->max_val;

    if (new_image->format == RGB_IMAGE) {
        new_image->nof_channels = RGB_NOF_CHANNELS;
    } else if (new_image->format == GRAYSCALE_IMAGE) {
        new_image->nof_channels = GRAYSCALE_NOF_CHANNELS;
    }

    in.ignore(1);

    unsigned int data_size = new_image->height 
                    * new_image->width * new_image->nof_channels; 

    new_image->data = new unsigned char[data_size];

    in.read((char*)new_image->data, data_size);
    in.close();

    return new_image;
}


ppm_image *convert_rgb_to_grayscale(ppm_image *original_image)
{
    if (original_image->format == GRAYSCALE_IMAGE)
        return nullptr;

    ppm_image *new_image = new ppm_image;

    new_image->format       = GRAYSCALE_IMAGE;
    new_image->nof_channels = GRAYSCALE_NOF_CHANNELS;
    new_image->height       = original_image->height;
    new_image->width        = original_image->width;
    new_image->max_val      = original_image->max_val;
    
    unsigned int data_size = new_image->height
                    * new_image->width * new_image->nof_channels;

    new_image->data = new unsigned char[data_size];

    for (unsigned int i = 0; i < data_size; i++) {
        unsigned char *pixel = original_image->data + 3 * i;

        new_image->data[i] = (*pixel * RED_COEF)
            + ((*pixel + 1) * GREEN_COEF) + (*(pixel + 2) * BLUE_COEF);
    }

    return new_image;
}


ppm_image *convert_grayscale_to_rgb(ppm_image *original_image)
{
    if (original_image->format == RGB_IMAGE)
        return nullptr;

    ppm_image *new_image = new ppm_image;

    new_image->format       = RGB_IMAGE;
    new_image->nof_channels = RGB_NOF_CHANNELS;
    new_image->width        = original_image->width;
    new_image->height       = original_image->height;
    new_image->max_val      = original_image->max_val;

    unsigned int data_size = new_image->width
                    * new_image->height * new_image->nof_channels;

    new_image->data = new unsigned char[data_size];

    for (unsigned int i = 0; i < data_size; i += 3) {
        new_image->data[i]     = original_image->data[i];
        new_image->data[i + 1] = original_image->data[i];
        new_image->data[i + 2] = original_image->data[i];
    }

    return new_image;
}


void write_image(const std::string &file_path, ppm_image *image)
{
    std::ofstream out(file_path, std::ios::binary);

    if (!out.is_open()) {
        std::cerr << "Could not open file " << file_path << std::endl;
        exit(EXIT_FAILURE);
    }

    out << image->format << "\n";
    out << image->width << " " << image->height << "\n";
    out << image->max_val << "\n";

    unsigned int data_size = image->width * image->height * image->nof_channels;

    out.write((char *)image->data, data_size);
    out.close();
}
