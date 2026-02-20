#include "pthreads_funcs.hpp"
#include "threadpool.hpp"
#include "pthread.h"

#include <iostream>

uint32_t calc_owed_rows(struct pthread_arg args) {
    uint32_t rows = args.rows / (num_threads - 1);
    if (args.rows % (num_threads - 1) > args.id)
        rows++;
    return rows;    
}

uint32_t calc_row_offset(struct pthread_arg args) {
    uint32_t offset = (args.rows / (num_threads - 1)) * args.id;

    offset += (args.id <= args.rows % (num_threads - 1)) ?
         args.id : args.rows % (num_threads - 1);

    return offset;
}

struct pthread_arg make_arg(uint32_t id, const void *data, 
                void *out, uint32_t rows, uint32_t cols, 
                sem_t *finished, void *sec_data, uint32_t cols2) {
    struct pthread_arg arg;
    arg.data = data;
    arg.cols = cols;
    arg.rows = rows;
    arg.id = id;
    arg.out = out;
    arg.finished = finished;
    arg.sec_data = sec_data;
    arg.cols2 = cols2;
    return arg;
}

void threadpool_launch(const void *data, 
                void *out, uint32_t rows, uint32_t cols, 
                ThreadPool *tp, void *(*function)(void *),
                void *sec_data, uint32_t cols2) {
    struct pthread_arg *args = new struct pthread_arg[num_threads - 1];

    sem_t finished;
    sem_init(&finished, PTHREAD_PROCESS_PRIVATE, 0);
    for (uint32_t i = 0; i < num_threads - 1; i++)
    {
        args[i] = make_arg(i, data, out, rows, cols, &finished, sec_data);
        struct thread_pool_arg task;
        task.arg = &(args[i]);
        task.function = function;
        tp->enqueue_task(task);
    }

    for (uint32_t i = 0; i < num_threads - 1; i++) 
        sem_wait(&finished); 

    sem_destroy(&finished);
    delete[] args;
}

void *process_convert_uchar_to_double(void *arg) {
    struct pthread_arg *args = (struct pthread_arg *)arg;
    double *out = (double *)(args->out);
    unsigned char *data = (unsigned char *)(args->data);
    uint32_t owed_rows = calc_owed_rows(*args);
    uint32_t row_offset = calc_row_offset(*args);
    for (uint32_t i = row_offset; i < row_offset + owed_rows; i++)
        out[i] = static_cast<double>(data[i]);
    sem_post(args->finished);
    return NULL;
}

void *process_convert_double_to_uchar(void *arg) {
    struct pthread_arg *args = (struct pthread_arg *)arg;
    unsigned char *out = (unsigned char *)(args->out);
    double *data = (double *)(args->data);
    uint32_t owed_rows = calc_owed_rows(*args);
    uint32_t row_offset = calc_row_offset(*args);
    for (uint32_t i = row_offset; i < row_offset + owed_rows; i++)
        out[i] = (data[i] < 0) ? 0 : static_cast<unsigned char>(data[i]);
    sem_post(args->finished);
    return NULL;
}

void *process_compute_mean_matrix(void *arg) {
    struct pthread_arg *args = (struct pthread_arg *)arg;
    double *out = (double*)(args->out);
    double *data = (double*)(args->data);
    out[args->id] = 0.0f;
    uint32_t owed_rows = calc_owed_rows(*args);
    uint32_t row_offset = calc_row_offset(*args);
    for (uint32_t i = row_offset; i < row_offset + owed_rows; i++)
        for (uint32_t j = 0; j < args->cols; j++)
            out[args->id] += data[i * args->cols + j];
    sem_post(args->finished);
    return NULL;
}

void *process_normalize_matrix(void *arg) {
    struct pthread_arg *args = (struct pthread_arg *)arg;
    double *out = (double *)(args->out);
    double *data = (double *)(args->data);
    uint32_t owed_rows = calc_owed_rows(*args);
    uint32_t row_offset = calc_row_offset(*args);
    for (uint32_t i = row_offset; i < row_offset + owed_rows; i++)
        for (uint32_t j = 0; j < args->cols; j++)
            out[i * args->cols + j] -= *data;
    sem_post(args->finished);
    return NULL;
}

void *process_transpose_matrix(void *arg) {
    struct pthread_arg *args = (struct pthread_arg *)arg;
    double *out = (double *)(args->out);
    double *data = (double *)(args->data);
    uint32_t owed_rows = calc_owed_rows(*args);
    uint32_t row_offset = calc_row_offset(*args);
    for (uint32_t i = row_offset; i < row_offset + owed_rows; i++)
        for (uint32_t j = 0; j < args->cols; j++)
            out[j * args->rows + i] = data[i * args->cols + j];
    sem_post(args->finished);
    return NULL;
}

void *process_compute_covariance_matrix(void *arg) {
    struct pthread_arg *args = (struct pthread_arg *)arg;
    double *out = (double *)(args->out);
    double *data = (double *)(args->data);
    uint32_t owed_rows  = calc_owed_rows(*args);
    uint32_t row_offset = calc_row_offset(*args);
    for (uint32_t i = row_offset; i < row_offset + owed_rows; i++)
        for (uint32_t j = 0; j < args->cols; j++)
            out[i * args->cols + j] *= *data;
    sem_post(args->finished);
    return NULL;
}

void *process_compute_dot_product(void *arg) {
    struct pthread_arg *args = (struct pthread_arg *)arg;
    double *out = (double*)(args->out);
    double *data = (double*)(args->data);
    double *sec_data = (double*)(args->sec_data);
    out[args->id] = 0.0f;
    uint32_t owed_rows = calc_owed_rows(*args);
    uint32_t row_offset = calc_row_offset(*args);
    for (uint32_t i = row_offset; i < row_offset + owed_rows; i++)
        for (uint32_t j = 0; j < args->cols; j++)
            out[args->id] += data[i * args->cols + j] * sec_data[i * args->cols + j];
    sem_post(args->finished);
    return NULL;
}


void *process_multiply_matrices(void *arg) {
    pthread_arg *args = (pthread_arg *)arg;

    const double *mat1 = (const double *)args->data;
    const double *mat2 = (const double *)args->sec_data;
    double *result     = (double *)args->out;
    
    uint32_t owed_rows  = calc_owed_rows(*args);
    uint32_t row_offset = calc_row_offset(*args);

    uint32_t cols  = args->cols;
    uint32_t cols2 = args->cols2;

    for (uint32_t i = row_offset; i < row_offset + owed_rows; i++) {
        for (uint32_t k = 0; k < cols; k++) {
            double temp = mat1[i * cols + k];
            for (uint32_t j = 0; j < cols2; j++) {
                result[i * cols2 + j] += temp * mat2[k * cols2 + j];

            }
        }
    }

    sem_post(args->finished);

    return nullptr;
}

void *process_lanczos_algorithm_1(void *arg) {
    struct pthread_arg *args = (struct pthread_arg *)arg;
    double *out = (double*)(args->out);
    double *data = (double*)(args->data);
    double *sec_data = (double*)(args->sec_data);
    uint32_t owed_rows = calc_owed_rows(*args);
    uint32_t row_offset = calc_row_offset(*args);
    for (uint32_t i = row_offset; i < row_offset + owed_rows; i++)
        for (uint32_t j = 0; j < args->cols; j++)
            out[i * args->cols + j] -= *(sec_data) * data[i * args->cols + j]; 
    sem_post(args->finished);
    return NULL;
}

void *process_lanczos_algorithm_2(void *arg) {
    struct pthread_arg *args = (struct pthread_arg *)arg;
    double *out = (double*)(args->out);
    double *data = (double*)(args->data);
    double *sec_data = (double*)(args->sec_data);
    uint32_t owed_rows = calc_owed_rows(*args);
    uint32_t row_offset = calc_row_offset(*args);
    for (uint32_t i = row_offset; i < row_offset + owed_rows; i++)
        for (uint32_t j = 0; j < args->cols; j++)
            out[i * args->cols + j] = data[i * args->cols + j] / *(sec_data);
    sem_post(args->finished);
    return NULL;
}

void *process_decompress_pca(void *arg) {
    struct pthread_arg *args = (struct pthread_arg *)arg;
    double *out = (double*)(args->out);
    double *data = (double*)(args->data);
    uint32_t owed_rows = calc_owed_rows(*args);
    uint32_t row_offset = calc_row_offset(*args);
    for (uint32_t i = row_offset; i < row_offset + owed_rows; i++)
        for (uint32_t j = 0; j < args->cols; j++)
            out[i * args->cols + j] += *data;
    sem_post(args->finished);
    return NULL;
}

void *process_generic(void *arg) {
    struct pthread_arg *args = (struct pthread_arg *)arg;
    // double *out = (double*)(args->out);
    // double *data = (double*)(args->data);
    uint32_t owed_rows = calc_owed_rows(*args);
    uint32_t row_offset = calc_row_offset(*args);
    for (uint32_t i = row_offset; i < row_offset + owed_rows; i++)
        for (uint32_t j = 0; j < args->cols; j++)
            // do smth
    sem_post(args->finished);
    return NULL;
}

