#include <stdio.h>
#include "thread_pool.h"

void print_hello(void *arg) {
    int id = *(int *)arg;
    printf("Task %d done\n", id);
}

int main(void) {
    thread_pool_t *pool = pool_create(4, 10);

    int ids[20];
    for (int i = 0; i < 20; i++) {
        ids[i] = i;
        pool_add_task(pool, print_hello, &ids[i]);
    }

    pool_shutdown(pool);
    return 0;
}
