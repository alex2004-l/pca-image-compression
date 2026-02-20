#include <chrono>
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <omp.h>
#include <string>

#include "ppm_image.hpp"
#include "pca_lib.hpp"

const char *output_img_dir   = "output/";
const char *output_comp_dir  = "output_compressed/";

uint32_t num_threads = 12;

int main(int argc, char **argv)
{
    // Unsync C and C++ stdio
    std::ios::sync_with_stdio(false);

    int nof_components = 20;

    // Check number of argumets
    if (argc < 2 || argc > 4) {
        std::cerr << "Usage : ./main <input_file> [optional <nof_components>]";
        return EXIT_FAILURE;
    }

    // Check if it s running on SLURM
    if (argc == 4) {
        num_threads = std::atoi(argv[3]);
    } else {
        if (const char* s = std::getenv("SLURM_CPUS_PER_TASK")) {
            int n = std::atoi(s);
            if (n > 0)
                num_threads = n;
        }
    }

    // Check if the directories for output exist, if not create them
    if (!std::filesystem::exists(output_img_dir)) {
        std::filesystem::create_directory(output_img_dir);
    }

    if (!std::filesystem::exists(output_comp_dir)) {
        std::filesystem::create_directory(output_comp_dir);
    }

    std::string input_path(argv[1]);
    ppm_image *img           = load_ppm_image(input_path);
    ppm_image *grayscale_img = convert_rgb_to_grayscale(img);

    if (argc == 3)
        nof_components = atoi(argv[2]);

    std::string input_image_name  = input_path.substr(input_path.find_last_of('/') + 1);
    std::string output_image_name = VARIANT_NAME + ("_" + input_image_name);
    std::string output_img_path   = output_img_dir  + output_image_name;
    std::string output_comp_path  = output_comp_dir + output_image_name;

    auto start = std::chrono::high_resolution_clock::now();
    compress_pca(output_comp_path, grayscale_img->data,
            grayscale_img->height, grayscale_img->width, nof_components);

    auto end       = std::chrono::high_resolution_clock::now();
    auto time_comp = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() * 1e-6;

    std::cout << VARIANT_NAME << "," << input_image_name << ","
              << grayscale_img->width << "," << grayscale_img->height << ","
              << time_comp << "," << nof_components << "," << num_threads << ",1" << std::endl;

    unsigned char *old_data = grayscale_img->data;
    unsigned char *new_data = decompress_pca(output_comp_path);

    grayscale_img->data = new_data;
    write_image(output_img_path, grayscale_img);
    
    delete[] old_data;

    delete[] img->data;
    delete   img;
    delete[] grayscale_img->data;
    delete   grayscale_img;

    return EXIT_SUCCESS;
}
