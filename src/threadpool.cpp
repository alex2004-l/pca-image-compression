#include <pthread.h>
#include <stdlib.h>
#include <queue>

#include "threadpool.hpp"

void *worker_thread(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;
    while (true) {
        pool->lock_queue_mutex();
        pool->await_task();
        if (pool->should_stop())
        {
            pool->unlock_queue_mutex();
            break;
        }
        struct thread_pool_arg task = pool->pop_task();
        pool->unlock_queue_mutex();
        task.function(task.arg);
    }
    return NULL;
}

ThreadPool::ThreadPool(int num_threads)
{
    this->num_threads = num_threads;
    threads = new pthread_t[num_threads];
    tasks = new std::queue<struct thread_pool_arg>();
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&task_notifier, NULL);
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, worker_thread, (void *)this);
    }
}

ThreadPool::~ThreadPool()
{
    pthread_mutex_lock(&queue_mutex);
    stop = true;
    pthread_cond_broadcast(&task_notifier);
    pthread_mutex_unlock(&queue_mutex);

    for (int i = 0; i < num_threads; i++) 
        pthread_join(threads[i], NULL);
    delete[] threads;
    delete tasks;
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&task_notifier);
}

void ThreadPool::lock_queue_mutex() {
    pthread_mutex_lock(&queue_mutex);
}

void ThreadPool::unlock_queue_mutex() {
    pthread_mutex_unlock(&queue_mutex);
}

void ThreadPool::await_task() {
    while (tasks->empty() && !stop) {
        pthread_cond_wait(&task_notifier, &queue_mutex);
    }
}

bool ThreadPool::should_stop() {
    return (stop && tasks->empty());
}

struct thread_pool_arg ThreadPool::pop_task() {
    struct thread_pool_arg task = tasks->front();
    tasks->pop();
    return task;
}

void ThreadPool::enqueue_task(struct thread_pool_arg task) {
    pthread_mutex_lock(&queue_mutex);
    tasks->emplace(task);
    pthread_cond_signal(&task_notifier);
    pthread_mutex_unlock(&queue_mutex);
}
