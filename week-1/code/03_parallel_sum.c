/*
 * 03_parallel_sum.c
 *
 * A more realistic scenario than a bare counter: we have a large
 * array of numbers, and we want to compute the sum across multiple
 * threads.
 *
 * The first (broken) approach in this file: split the array into
 * chunks, give each thread a chunk, and have each thread add its
 * chunk's values into a shared `total`.
 *
 * The expected sum is well-defined: we fill the array with all 1s,
 * so the sum is exactly ARRAY_SIZE. After running, compare what we
 * got vs what we expected. The mismatch is the race condition.
 *
 * Goal of this example:
 *   - Show that race conditions are not just about toy counters.
 *   - Real parallel work can have the same bug.
 *   - Set the stage for Week 2, where we'll fix it properly.
 *
 * Compile:
 *   gcc -Wall -pthread 03_parallel_sum.c -o parallel_sum
 *
 * Run:
 *   ./parallel_sum
 *
 * Modify NUM_THREADS and re-run. More threads -> usually more error.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define ARRAY_SIZE  10000000
#define NUM_THREADS 4

int *array;             /* heap-allocated, shared between threads */
long total = 0;         /* SHARED accumulator — this is the bug */

typedef struct {
    int thread_id;
    long start_index;
    long end_index;     /* exclusive */
} thread_arg_t;

void *partial_sum(void *arg) {
    thread_arg_t *t = (thread_arg_t *)arg;
    for (long i = t->start_index; i < t->end_index; i++) {
        total += array[i];      /* race condition lives here */
    }
    return NULL;
}

int main(void) {
    /* Allocate and fill the array with 1s so the expected sum is obvious. */
    array = malloc(ARRAY_SIZE * sizeof(int));
    if (!array) {
        perror("malloc");
        return 1;
    }
    for (long i = 0; i < ARRAY_SIZE; i++) {
        array[i] = 1;
    }

    pthread_t threads[NUM_THREADS];
    thread_arg_t args[NUM_THREADS];
    long chunk = ARRAY_SIZE / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].thread_id   = i;
        args[i].start_index = i * chunk;
        args[i].end_index   = (i == NUM_THREADS - 1) ? ARRAY_SIZE
                                                     : (i + 1) * chunk;
        pthread_create(&threads[i], NULL, partial_sum, &args[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Computed sum: %ld\n", total);
    printf("Expected:     %d\n",  ARRAY_SIZE);

    if (total == ARRAY_SIZE) {
        printf("Looks correct — but try running again, possibly several times.\n");
    } else {
        printf("Off by %ld. Race condition strikes again.\n", (long)ARRAY_SIZE - total);
    }

    free(array);
    return 0;
}
