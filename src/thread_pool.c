#include <stdlib.h>
#include "thread_pool.h"

static void* worker_thread(void *arg) {
    thread_pool_t *pool = (thread_pool_t *)arg;

    while (1) {
        pthread_mutex_lock(&pool->lock);

        while (pool->queue_size == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->not_empty, &pool->lock);
        }

        if (pool->queue_size == 0 && pool->shutdown) {
            pthread_mutex_unlock(&pool->lock);
            return NULL;
        }

        task_t task = pool->task_queue[pool->queue_front];
        pool->queue_front = (pool->queue_front + 1) % pool->queue_capacity;
        pool->queue_size--;

        pthread_cond_signal(&pool->not_full);
        pthread_mutex_unlock(&pool->lock);

        task.function(task.arg);
    }
    return NULL;
}

thread_pool_t* pool_create(int num_threads, int queue_capacity) {
    thread_pool_t *pool = malloc(sizeof(thread_pool_t));
    if (!pool) return NULL;

    pool->num_threads = num_threads;
    pool->queue_capacity = queue_capacity;
    pool->queue_size = 0;
    pool->queue_front = 0;
    pool->queue_rear = 0;
    pool->shutdown = 0;

    pool->task_queue = malloc(queue_capacity * sizeof(task_t));
    if (!pool->task_queue) {
        free(pool);
        return NULL;
    }

    pool->threads = malloc(num_threads * sizeof(pthread_t));
    if (!pool->threads) {
        free(pool->task_queue);
        free(pool);
        return NULL;
    }

    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->not_empty, NULL);
    pthread_cond_init(&pool->not_full, NULL);

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&pool->threads[i], NULL, worker_thread, pool);
    }

    return pool;
}

int pool_add_task(thread_pool_t *pool, void (*function)(void *), void *arg) {
    pthread_mutex_lock(&pool->lock);

    while (pool->queue_size == pool->queue_capacity && !pool->shutdown) {
        pthread_cond_wait(&pool->not_full, &pool->lock);
    }

    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->lock);
        return -1;
    }

    task_t task;
    task.function = function;
    task.arg = arg;
    pool->task_queue[pool->queue_rear] = task;
    pool->queue_rear = (pool->queue_rear + 1) % pool->queue_capacity;
    pool->queue_size += 1;

    pthread_cond_signal(&pool->not_empty);
    pthread_mutex_unlock(&pool->lock);
    return 0;
}

void pool_shutdown(thread_pool_t *pool) {
    pthread_mutex_lock(&pool->lock);
    pool->shutdown = 1;
    pthread_cond_broadcast(&pool->not_empty);
    pthread_cond_broadcast(&pool->not_full);
    pthread_mutex_unlock(&pool->lock);

    for (int i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->not_full);
    pthread_cond_destroy(&pool->not_empty);

    free(pool->task_queue);
    free(pool->threads);
    free(pool);
}
