#include "mpi_pca.hpp"

void bcast_string_send(std::string *string, MPI_Comm communicator, int origin_rank, int rank) {
    int len;
    if (origin_rank == rank) {
        len = (*string).size();
        MPI_Bcast(&len, 1, MPI_INT, origin_rank, communicator);
    } else {
        MPI_Bcast(&len, 1, MPI_INT, origin_rank, communicator);
        (*string).resize(len);
    }
    MPI_Bcast(const_cast<char*>((*string).data()), len, MPI_CHAR, origin_rank, communicator);
}

void prepare_block_dimensions(ppm_image *grayscale_img, int tasks_per_side, int *block_heights, 
                            int *block_widths, int *coordinator_block_size, 
                            int *coordinated_line_offsets, int *coordinator_block_offsets) {
    uint32_t height = grayscale_img->height;
    uint32_t width = grayscale_img->width;

    for (int i = 0; i < tasks_per_side ; i++) {
        coordinator_block_size[i] = width;
    }
    coordinated_line_offsets[0] = 0;
    coordinator_block_offsets[0] = 0;

    for (int i = 0; i < tasks_per_side - 1; i++) {
        block_heights[i] = height / (tasks_per_side - i);
        coordinator_block_size[i] *= block_heights[i];
        coordinator_block_offsets[i + 1] = coordinator_block_offsets[i] + coordinator_block_size[i]; 
        height -= block_heights[i];
    }
    block_heights[tasks_per_side - 1] = height;
    coordinator_block_size[tasks_per_side - 1] *= height;
    
    for (int i = 0; i < tasks_per_side - 1; i++) {
        block_widths[i] = width / (tasks_per_side - i);
        coordinated_line_offsets[i + 1] = coordinated_line_offsets[i] + block_widths[i]; 
        width -= block_widths[i];
    }
    block_widths[tasks_per_side - 1] = width;
}

void share_image_partition(int *nof_components, int tasks_per_side, int *block_heights,
                            int *block_widths, MPI_Comm communicator) {
    MPI_Bcast(nof_components, 1, MPI_INT, ROOT, communicator);
    MPI_Bcast(block_heights, tasks_per_side, MPI_INT, ROOT, communicator);
    MPI_Bcast(block_widths, tasks_per_side, MPI_INT, ROOT, communicator);
}

void share_block_dimensions(int tasks_per_side, int *coordinator_block_size, int *coordinator_block_offsets,
                            int *coordinated_line_offsets, uint32_t *width, MPI_Comm communicator) {
    MPI_Bcast(coordinator_block_size, tasks_per_side, MPI_INT, ROOT, communicator);
    MPI_Bcast(coordinator_block_offsets, tasks_per_side, MPI_INT, ROOT, communicator);
    MPI_Bcast(coordinated_line_offsets, tasks_per_side, MPI_INT, ROOT, communicator);                
    MPI_Bcast(width, 1, MPI_UINT32_T, ROOT, communicator);
}
