# Thread Pool (C / POSIX Threads)

A fixed-size thread pool implemented in C using `pthread`, with a bounded,
circular-buffer task queue synchronized via mutexes and condition variables.

## Features

- Fixed number of worker threads, each running an independent loop
- Bounded task queue (circular buffer) — producers block if the queue is full
- Condition variables (`not_empty` / `not_full`) instead of busy-waiting, so
  idle workers/producers consume no CPU while blocked
- Graceful shutdown — queued tasks are drained to completion before worker
  threads exit and are joined
- Generic task interface: any `void (*)(void *arg)` function can be submitted

## Usage

```c
#include "thread_pool.h"

void my_task(void *arg) {
    int id = *(int *)arg;
    printf("running task %d\n", id);
}

int main(void) {
    thread_pool_t *pool = pool_create(4 /* workers */, 10 /* queue capacity */);

    int ids[20];
    for (int i = 0; i < 20; i++) {
        ids[i] = i;
        pool_add_task(pool, my_task, &ids[i]);
    }

    pool_shutdown(pool); // waits for all tasks to finish, then cleans up
    return 0;
}
```

## Build & run the example

```
make run
```

## Design notes

- The queue is a circular buffer, avoiding the need to shift elements on
  removal (`O(1)` insert/remove instead of `O(n)`).
- `pool_add_task` blocks (rather than dropping tasks) when the queue is full,
  applying natural backpressure to producers.
- Shutdown is graceful: setting `shutdown = 1` doesn't stop workers
  mid-queue — they keep draining tasks until the queue is empty, and only
  then exit. This is checked with `queue_size == 0 && shutdown`, not
  `shutdown` alone.
- Two separate condition variables (`not_empty`, `not_full`) are used
  instead of one, so workers waiting for tasks aren't woken by producer-side
  events and vice versa.

## Tested with

- Valgrind (`--leak-check=full`) — no leaks, no errors
- `gcc -Wall -Wextra` — no warnings
