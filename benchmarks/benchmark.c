#include <stdio.h>
#include "thread_pool.h"

int is_prime(int n) {
    if (n <= 1) return 0;          
    for (int d = 2; d < n; d++) {  
        if (n % d == 0) {          
            return 0;              
        }
    }
    return 1;                     
}

int count_primes_below(int limit){
    int count = 0;
    for(int i=2;i<limit;i++){
        if(is_prime(i)){
            count++;
        }
    }
    return count;
}

void prime_task(void *arg) {
    int limit = *(int* )arg ;            
    int result = count_primes_below(limit);
    printf("primes below %d: %d\n", limit, result);  // prevent smart compiler from ignoring call to func
                                                     // otherwise benchmark measure "nothing" as smart compiler knows results are not used if not print etc
}

#define NUM_TASKS 200
#define PRIME_LIMIT 100000
#define NUM_TRIALS 5

int main(void) {
    int thread_counts[] = {1, 2, 4, 8, 16, 32};
    int num_configs = 6;

    for (int c = 0; c < num_configs; c++) {
        int num_threads = thread_counts[c];
        double total_time = 0.0;

        for (int trial = 0; trial < NUM_TRIALS; trial++) {

            thread_pool_t *pool = pool_create(num_threads, 250);

            struct timespec start, end;
            clock_gettime(CLOCK_MONOTONIC, &start);

            int args[NUM_TASKS];
            for(int i=0; i < NUM_TASKS; i++){
                args[i] = PRIME_LIMIT;
                pool_add_task(pool,prime_task,&args[i]);
            }


            pool_shutdown(pool);

            clock_gettime(CLOCK_MONOTONIC, &end);
            double elapsed = (end.tv_sec - start.tv_sec)
                            + (end.tv_nsec - start.tv_nsec) / 1e9;

            total_time += elapsed;
        }

        double avg_time = total_time / NUM_TRIALS;
        printf("threads=%d avg_time=%.4f s\n", num_threads, avg_time);
    }

    return 0;
}
