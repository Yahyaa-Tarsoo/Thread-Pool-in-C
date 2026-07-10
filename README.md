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
  up when a task is added — using condition variables instead of a
  `while(true) { check flag }` loop
- If the queue is full, `pool_add_task` blocks the caller until a slot opens
  up, instead of dropping tasks or growing unbounded
- Shutdown is "graceful" — if you call `pool_shutdown()` while tasks are
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

## Benchmark

To actually measure whether this thing does what it's supposed to (rather
than just trusting it), I benchmarked it with a CPU-bound synthetic workload:
counting primes below 100,000 using plain trial division, 200 tasks per run,
averaged over 5 trials per thread count. Wall-clock time measured with
`clock_gettime(CLOCK_MONOTONIC, ...)`, timed from right after pool creation
to right after `pool_shutdown()` returns (so only actual task-processing
time is measured, not pool setup).

Machine: Intel Core i7-13620H (6 performance cores + 4 efficiency cores, 16
logical threads via Hyper-Threading on the P-cores).

| Threads | Avg time (s) | Speedup | Efficiency |
|---|---|---|---|
| 1  | 124.80 | 1.00x | 100%  |
| 2  | 62.64  | 1.99x | 99.7% |
| 4  | 31.56  | 3.95x | 98.8% |
| 8  | 20.49  | 6.09x | 76.1% |
| 16 | 15.35  | 8.13x | 50.8% |
| 32 | 15.26  | 8.18x | 25.6% |

*(speedup = time at 1 thread / time at N threads. efficiency = speedup / N.)*

**What this shows:** scaling is close to perfect (98-100% efficiency) up
through 4 threads, then drops off past 8, and completely flattens at 16-32
(going from 16 to 32 threads barely changes anything: 15.35s vs 15.26s).

This isn't a flaw in the thread pool, it's a hardware ceiling. My CPU only
has 6 "real" performance cores. Two of those can each juggle 2 threads at
once, so the chip reports 16 logical threads total, but that's not the same
as 16 independent cores. Once every task queued up needs a thread and all 6
performance cores are already busy, additional threads are just splitting
time on cores that are already loaded, or landing on the weaker efficiency
cores, so each extra thread buys less and less. Past 16, there's nothing
left to hand work to, which is exactly why 16 and 32 threads perform almost
identically.

Reproduce it yourself:
```
gcc -Wall -Wextra -Iinclude -pthread src/thread_pool.c benchmarks/benchmark.c -o benchmark
./benchmark
```

## What's next

I'm using this as the base for a bigger project — a multithreaded file
indexer/search tool where each file gets submitted as a task to this pool.
Also have a `poll()`-based TCP server (a different project) that I might
eventually combine this with, so requests can be handled off the main I/O
thread instead of blocking it.
