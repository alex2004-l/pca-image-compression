#include <chrono>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <omp.h>
#include <string>

#include "mpi_pca.hpp"
#include "ppm_image.hpp"
#include "pca_lib.hpp"
#include <cmath>

const char *output_img_dir   = "output/";
const char *output_comp_dir  = "output_compressed/";

uint32_t num_threads = 12;

int main(int argc, char **argv)
{
    // Unsync C and C++ stdio
    std::ios::sync_with_stdio(false);

    int numtasks, rank, len;
    char hostname[MPI_MAX_PROCESSOR_NAME];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Get_processor_name(hostname, &len);

    int coordinator_rank;
    int coordinated_rank;
    MPI_Comm coordinator_communicator;
    MPI_Comm coordinated_communicator;
    MPI_Comm reduced_communicator;

    int nof_components = 20;

    // Check number of argumets
    if (argc < 2 || argc > 4) {
        if (rank == ROOT) {
            std::cerr << "Usage : ./main <input_file> [optional <nof_components> <num_threads>]";
        }
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
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

    ppm_image *grayscale_img = nullptr;

    std::string output_img_path;
    std::string output_comp_path;

    double *time_comps = nullptr;
    double time_comp;


    std::string input_image_name  = input_path.substr(input_path.find_last_of('/') + 1);
    std::string output_image_name = VARIANT_NAME + ("_" + input_image_name);

    int tasks_per_side = floor(sqrt(numtasks));
    MPI_Comm_split(MPI_COMM_WORLD, (rank < tasks_per_side * tasks_per_side) ? 1 : MPI_UNDEFINED, rank, &reduced_communicator);

    if (rank < tasks_per_side * tasks_per_side)
    {
        int *block_heights = new int[tasks_per_side];
        int *block_widths = new int[tasks_per_side];
        int *coordinator_block_size = new int[tasks_per_side];
        int *coordinator_block_offsets = new int[tasks_per_side];
        int *coordinated_line_offsets = new int[tasks_per_side];
        ppm_image *local_img = new ppm_image;
        ppm_image *coordinator_img = nullptr;
        ppm_image *img = nullptr;

        if (rank == ROOT) {
            img = load_ppm_image(input_path);
            grayscale_img = convert_rgb_to_grayscale(img);

            if (argc == 3)
                nof_components = atoi(argv[2]);
        }

        auto start = std::chrono::high_resolution_clock::now();

        if (rank == ROOT) {
            time_comps = new double[tasks_per_side * tasks_per_side];
            
            prepare_block_dimensions(grayscale_img, tasks_per_side, block_heights, block_widths,
                    coordinator_block_size, coordinated_line_offsets, coordinator_block_offsets);

            coordinated_rank = rank % tasks_per_side;
            coordinator_rank = rank / tasks_per_side;
            coordinator_img = new ppm_image;
            coordinator_img->data = new unsigned char[coordinator_block_size[coordinator_rank]];
            coordinator_img->height = block_heights[coordinator_rank];
            coordinator_img->width = grayscale_img->width;

            share_image_partition(&nof_components, tasks_per_side, block_heights, block_widths, reduced_communicator);
            MPI_Comm_split(reduced_communicator, rank / tasks_per_side, rank % tasks_per_side, &coordinated_communicator); // 0 1 2
            MPI_Comm_split(reduced_communicator, rank % tasks_per_side, rank / tasks_per_side, &coordinator_communicator); // 0 3 6
            share_block_dimensions(tasks_per_side, coordinator_block_size, coordinator_block_offsets,
                    coordinated_line_offsets, &(grayscale_img->width), coordinator_communicator);

            MPI_Scatterv(grayscale_img->data, coordinator_block_size, coordinator_block_offsets, MPI_CHAR, 
                coordinator_img->data, coordinator_block_size[coordinator_rank], MPI_CHAR,
                ROOT, coordinator_communicator);

                
            delete[] img->data;
            delete   img;
            output_img_path = output_img_dir + output_image_name;
        } else {
            share_image_partition(&nof_components, tasks_per_side, block_heights, block_widths, reduced_communicator); 
            coordinated_rank = rank % tasks_per_side;
            coordinator_rank = rank / tasks_per_side;
            MPI_Comm_split(reduced_communicator, rank / tasks_per_side, rank % tasks_per_side, &coordinated_communicator); // 0 1 2
            MPI_Comm_split(reduced_communicator, (coordinated_rank == ROOT) ? (rank % tasks_per_side) : MPI_UNDEFINED, rank / tasks_per_side, &coordinator_communicator); // 0 3 6
            if (coordinator_communicator != MPI_COMM_NULL) {
                coordinator_img = new ppm_image;
                share_block_dimensions(tasks_per_side, coordinator_block_size, coordinator_block_offsets,
                        coordinated_line_offsets, &(coordinator_img->width), coordinator_communicator);

                coordinator_img->data = new unsigned char[coordinator_block_size[coordinator_rank]];
                coordinator_img->height = block_heights[coordinator_rank];

                MPI_Scatterv(NULL, NULL, NULL, NULL, 
                    coordinator_img->data, coordinator_block_size[coordinator_rank], MPI_CHAR,
                    ROOT, coordinator_communicator);
            }
        }

        local_img->width = block_widths[coordinated_rank];
        local_img->height = block_heights[coordinator_rank];
        local_img->data = new unsigned char[local_img->width * local_img->height];

        if (coordinated_rank == ROOT) {
            for (int i = 0; i < block_heights[coordinator_rank]; i++) {
                MPI_Scatterv(coordinator_img->data + i * coordinator_img->width, block_widths, coordinated_line_offsets, MPI_CHAR,
                    local_img->data + i * block_widths[coordinated_rank], block_widths[coordinated_rank], MPI_CHAR,
                    ROOT, coordinated_communicator);
            }
            
        } else {
            for (int i = 0; i < block_heights[coordinator_rank]; i++) {
                MPI_Scatterv(NULL, NULL, NULL, NULL,
                    local_img->data + i * block_widths[coordinated_rank], block_widths[coordinated_rank], MPI_CHAR,
                    ROOT, coordinated_communicator);
            }
        }

        if (rank == ROOT) {
            output_comp_path = output_comp_dir + output_image_name;
        }

        bcast_string_send(&output_comp_path, reduced_communicator, ROOT, rank);
        output_comp_path = output_comp_path + "_" + std::to_string(rank);

        compress_pca(output_comp_path, local_img->data,
                local_img->height, local_img->width, nof_components);
        
        auto end = std::chrono::high_resolution_clock::now();
        time_comp = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() * 1e-6;

        unsigned char *old_data = local_img->data;
        unsigned char *new_data = decompress_pca(output_comp_path);
        local_img->data = new_data;

        delete[] old_data; 

        if (coordinated_rank == ROOT) {
            for (int i = 0; i < block_heights[coordinator_rank]; i++) {
                MPI_Gatherv(local_img->data + i * block_widths[coordinated_rank], block_widths[coordinated_rank], MPI_CHAR,
                    coordinator_img->data + i * coordinator_img->width, block_widths, coordinated_line_offsets, MPI_CHAR,
                    ROOT, coordinated_communicator);
            }

            if (rank == ROOT) {
                MPI_Gatherv(coordinator_img->data, coordinator_block_size[coordinator_rank], MPI_CHAR,
                    grayscale_img->data, coordinator_block_size, coordinator_block_offsets, MPI_CHAR,
                    ROOT, coordinator_communicator);
            } else {
                MPI_Gatherv(coordinator_img->data, coordinator_block_size[coordinator_rank], MPI_CHAR,
                    NULL, NULL, NULL, NULL,
                    ROOT, coordinator_communicator);
            }
            delete[] coordinator_img->data;
            delete coordinator_img;
            
        } else {
            for (int i = 0; i < block_heights[coordinator_rank]; i++) {
                MPI_Gatherv(local_img->data + i * block_widths[coordinated_rank], block_widths[coordinated_rank], MPI_CHAR,
                    NULL, NULL, NULL, NULL,
                    ROOT, coordinated_communicator);
            }
        }

        delete[] local_img->data;
        delete local_img;

        MPI_Gather(&time_comp, 1, MPI_DOUBLE, time_comps, 1, MPI_DOUBLE, ROOT, reduced_communicator);

        if (rank == ROOT) {
            write_image(output_img_path, grayscale_img);

            double max_time_comp = time_comps[0];
            for (int i = 1; i < tasks_per_side * tasks_per_side; i++) {
                max_time_comp = max_time_comp > time_comps[i] ? max_time_comp : time_comps[i];
            }
            delete[] time_comps;
            
            std::cout << VARIANT_NAME << "," << input_image_name << "," << grayscale_img->width << ","
                << grayscale_img->height << "," << max_time_comp << "," << nof_components << ","
                << num_threads << "," << numtasks << std::endl;
            
            delete[] grayscale_img->data;
            delete   grayscale_img;
        }
    }
    
    MPI_Finalize();
    return EXIT_SUCCESS;
}
