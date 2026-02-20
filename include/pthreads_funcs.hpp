#ifndef __PTHREAD_FUNCS_HPP
#define __PTHREAD_FUNCS_HPP

#include <stdint.h>
#include <semaphore.h>
#include "threadpool.hpp"

extern uint32_t num_threads;

uint32_t calc_owed_rows(struct pthread_arg args);

uint32_t calc_row_offset(struct pthread_arg args);

struct pthread_arg make_arg(uint32_t id, const void *data, 
                void *out, uint32_t rows, uint32_t cols, 
                sem_t *finished, void *sec_data = nullptr, uint32_t cols2 = 0);

void threadpool_launch(const void *data, 
                void *out, uint32_t rows, uint32_t cols, 
                ThreadPool *tp, void *(*function)(void *),
                void *sec_data = nullptr, uint32_t cols2 = 0);

void *process_convert_uchar_to_double(void *arg);

void *process_convert_double_to_uchar(void *arg);

void *process_compute_mean_matrix(void *arg);

void *process_normalize_matrix(void *arg);

void *process_transpose_matrix(void *arg);

void *process_compute_covariance_matrix(void *arg);

void *process_compute_dot_product(void *arg);

void *process_multiply_matrices(void *arg);

void *process_lanczos_algorithm_1(void *arg);

void *process_lanczos_algorithm_2(void *arg);

void *process_decompress_pca(void *arg);

#endif /* __PTHREAD_FUNCS_HPP */

