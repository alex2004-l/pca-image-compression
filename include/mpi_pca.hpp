#ifndef __MPI_PCA_HPP
#define __MPI_PCA_HPP

#include <mpi.h>
#include "ppm_image.hpp"

#define ROOT 0

/*
 * Wrapper function for sending std::strings over MPI broacasts.
 * @param string          the string containing the data to be shared for the origin, and the place to put the data for every other rank
 * @param communicator    the communicator over which the broadcast takes place
 * @param origin_rank     the rank of the data source within the given communicator
 * @param rank            the rank of the current process within the given communicator
 */
void bcast_string_send(std::string *string, MPI_Comm communicator, int origin_rank, int rank);

/*
 * Function for calculating the image partition into tasks_per_side by tasks_per_side blocks
 * @param grayscale_img                 - ppm object containing the image to-be partitioned
 * @param tasks_per_side                - integer determining the number of parts each side is split into 
 * @param block_heights                 - integer array that stores the lengths of vertical partitions
 * @param block_widths                  - integer array that stores the lengths of horizontal partitions
 * @param coordinator_block_size        - integer array that stores the full blocksize for an image's vertical partitions
 * @param coordinated_line_offsets      - integer array that stores the offset to the begining of an image's vertical partitions
 * @param coordinator_block_offsets     - integer array that stores the offset within a given line for an image's horizontal partitions
 */
void prepare_block_dimensions(ppm_image *grayscale_img, int tasks_per_side, int *block_heights, 
                            int *block_widths, int *coordinator_block_size, 
                            int *coordinated_line_offsets, int *coordinator_block_offsets);

/*
 * Wrapper function for sending global parition metadata
 * @param nof_components    - number of components in the picture
 * @param tasks_per_side    - integer determining the number of parts each side is split into 
 * @param block_heights     - integer array that stores the lengths of vertical partitions
 * @param block_widths      - integer array that stores the lengths of horizontal partitions
 * @param communicator      - the communicator over which the broadcast takes place
 */
void share_image_partition(int *nof_components, int tasks_per_side, int *block_heights,
                            int *block_widths, MPI_Comm communicator);

/*
 * Wrapper function for sending coordinator parition metadata
 * @param tasks_per_side                - integer determining the number of parts each side is split into 
 * @param coordinator_block_size        - integer array that stores the full blocksize for an image's vertical partitions
 * @param coordinator_block_offsets     - integer array that stores the offset within a given line for an image's horizontal partitions
 * @param coordinated_line_offsets      - integer array that stores the offset to the begining of an image's vertical partitions
 * @param width                         - length of a full line of the image
 * @param communicator                  - the communicator over which the broadcast takes place
 */
void share_block_dimensions(int tasks_per_side, int *coordinator_block_size, int *coordinator_block_offsets,
                            int *coordinated_line_offsets, uint32_t *width, MPI_Comm communicator);


#endif /* __MPI_PCA_HPP */
