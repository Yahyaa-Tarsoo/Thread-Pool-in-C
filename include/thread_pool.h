#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>

typedef struct {
    void (*function)(void *arg);
    void *arg;
} task_t;

typedef struct {
    pthread_t *threads;
    int num_threads;

    task_t *task_queue;       // circular buffer
    int queue_capacity;
    int queue_size;
    int queue_front;
    int queue_rear;

    pthread_mutex_t lock;
    pthread_cond_t  not_empty;
    pthread_cond_t  not_full;

    int shutdown;
} thread_pool_t;

// Creates a thread pool with `num_threads` workers and a task queue that can
// hold up to `queue_capacity` pending tasks. Returns NULL on allocation failure.
thread_pool_t* pool_create(int num_threads, int queue_capacity);

// Submits a task to the pool. Blocks if the queue is full. Returns 0 on
// success, -1 if the pool is shutting down and no longer accepts work.
int pool_add_task(thread_pool_t *pool, void (*function)(void *arg), void *arg);

// Signals shutdown, waits for all queued tasks to finish, joins every worker
// thread, and frees all resources associated with the pool.
void pool_shutdown(thread_pool_t *pool);

#endif // THREAD_POOL_H
