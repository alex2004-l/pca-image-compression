#ifndef __THREADPOOL_HPP
#define __THREADPOOL_HPP

#include <stdint.h>
#include <stdlib.h>
#include <semaphore.h>
#include <queue>

extern uint32_t num_threads;

struct pthread_arg
{
    uint32_t    id;
    const void *data;
    const void *sec_data;
    void       *out;
    uint32_t    rows;
    uint32_t    cols;
    uint32_t    cols2;
    sem_t      *finished;
};

struct thread_pool_arg 
{
    void *arg;
    void *(*function)(void *);
};

class ThreadPool
{
private:
    int num_threads;
    pthread_t *threads;
    std::queue<struct thread_pool_arg> *tasks;
    pthread_mutex_t queue_mutex;
    pthread_cond_t task_notifier;
    bool stop = false;
    
public:

    ThreadPool(int num_threads);
    ~ThreadPool();
    void lock_queue_mutex();
    void unlock_queue_mutex();
    void await_task();
    bool should_stop();
    thread_pool_arg pop_task();
    void enqueue_task(thread_pool_arg task);
};

#endif /* __THREADPOOL_HPP */
