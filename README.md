# Thread Pool in C

A thread pool I built in C using pthreads to actually learn how mutexes and
condition variables work, instead of just reading about them. I'm a CompE
freshman at NYU Tandon and wanted something beyond class assignments to
understand concurrency at a lower level before I hit OS/architecture courses.

## What it does

- Spins up a fixed number of worker threads that pull tasks off a shared
  queue and run them
- The queue is a circular buffer, so tasks are added/removed in O(1) without
  shifting elements around
- Workers sleep (don't spin/busy-wait) when there's nothing to do, and wake
  up when a task is added , using condition variables instead of a
  `while(true) { check flag }` loop
- If the queue is full, `pool_add_task` blocks the caller until a slot opens
  up, instead of dropping tasks or growing unbounded
- Shutdown is "graceful" i.e if you call `pool_shutdown()` while tasks are
  still queued, workers finish everything left before exiting, they don't
  just stop mid-queue
- Tasks are generic: submit any function that takes a `void *`, so the pool
  doesn't care what kind of work it's running

## Why I built it this way

The hardest part for me wasn't the thread pool logic itself, it was
understanding *why* certain things have to be done a specific way, like:
- why `pthread_cond_wait` needs the mutex passed into it (otherwise there's
  a gap where a wakeup signal can get missed entirely — a "lost wakeup")
- why the wait condition has to be a `while` loop and not an `if` (multiple
  workers can wake up for the same signal, so you have to re-check)
- why you unlock the mutex before actually running a task, not after
  (otherwise only one task could ever run at a time, defeating the whole
  point of having multiple threads)

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

## Testing

Ran it through Valgrind (`--leak-check=full`) — no leaks, no errors. Also
compiles clean with `gcc -Wall -Wextra`, no warnings.

## What's next

I'm using this as the base for a bigger project — a multithreaded file
indexer/search tool where each file gets submitted as a task to this pool.
Also have a `poll()`-based TCP server (a different project) that I might
eventually combine this with, so requests can be handled off the main I/O
thread instead of blocking it.
