/*
 * 02_return_values.c
 *
 * How to return a result from a thread to the thread that created it.
 *
 * There are two clean approaches:
 *
 *   APPROACH A — write results into the argument struct (recommended):
 *     Pass a pointer to a struct that has both input and output fields.
 *     The thread writes its result into the struct's output fields.
 *     After joining, the caller reads the output fields directly.
 *     No extra malloc, no extra free, no void** casting.
 *
 *   APPROACH B — malloc and return:
 *     The thread allocates a result on the heap and returns the pointer.
 *     pthread_join captures it via a void**.
 *     The caller must free the memory.
 *     More flexible, but more bookkeeping.
 *
 * WRONG approach (shown in comments):
 *     Returning a pointer to a local variable. The local is gone when
 *     the thread returns. The pointer is dangling. This will sometimes
 *     appear to work (the memory hasn't been overwritten yet), which
 *     makes this bug particularly insidious.
 *
 * This program computes statistics (sum, min, max) on a large array
 * in parallel using NUM_THREADS threads. Each thread processes a chunk
 * of the array and returns its partial results.
 *
 * Compile:
 *   gcc -Wall -pthread -std=c11 02_return_values.c -o return_values
 *
 * Run:
 *   ./return_values
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>

#define ARRAY_SIZE   10000000
#define NUM_THREADS  4

/* ------------------------------------------------------------------ */
/* The struct that holds both input (chunk boundaries) and output      */
/* (partial results). One per thread, allocated in main.               */
/* ------------------------------------------------------------------ */
typedef struct {
    /* --- Inputs: set by main before creating the thread --- */
    long  start;
    long  end;          /* exclusive */
    int  *array;

    /* --- Outputs: written by the thread, read by main after join --- */
    long  partial_sum;
    int   partial_min;
    int   partial_max;
} chunk_args_t;

/* ------------------------------------------------------------------ */
/* APPROACH A — write results into the argument struct                  */
/* ------------------------------------------------------------------ */
void *compute_stats(void *arg) {
    chunk_args_t *d = (chunk_args_t *)arg;

    long sum = 0;
    int  mn  = INT_MAX;
    int  mx  = INT_MIN;

    for (long i = d->start; i < d->end; i++) {
        int v = d->array[i];
        sum += v;
        if (v < mn) mn = v;
        if (v > mx) mx = v;
    }

    d->partial_sum = sum;
    d->partial_min = mn;
    d->partial_max = mx;
    return NULL;   /* main reads results from the struct, no return value needed */
}

/* ------------------------------------------------------------------ */
/* APPROACH B — malloc a result and return it                           */
/* (Shown separately so you can compare the two styles.)               */
/* ------------------------------------------------------------------ */
typedef struct { long sum; } heap_result_t;

void *sum_only(void *arg) {
    chunk_args_t *d = (chunk_args_t *)arg;
    long sum = 0;
    for (long i = d->start; i < d->end; i++)
        sum += d->array[i];

    heap_result_t *result = malloc(sizeof(heap_result_t));
    result->sum = sum;
    return result;    /* caller captures via pthread_join and must free */
}

/*
 * WRONG — returning a pointer to a local variable. DO NOT DO THIS.
 *
 * void *bad_worker(void *arg) {
 *     long local_sum = 42;
 *     return &local_sum;    // local_sum dies when this function returns!
 * }
 */

int main(void) {
    /* Build the array — all 1s so expected sum == ARRAY_SIZE */
    int *array = malloc(ARRAY_SIZE * sizeof(int));
    for (long i = 0; i < ARRAY_SIZE; i++)
        array[i] = 1;

    long chunk = ARRAY_SIZE / NUM_THREADS;
    pthread_t     threads[NUM_THREADS];
    chunk_args_t  args[NUM_THREADS];

    /* ---- Approach A ------------------------------------------- */
    printf("=== Approach A: results written into argument struct ===\n");

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].array = array;
        args[i].start = i * chunk;
        args[i].end   = (i == NUM_THREADS - 1) ? ARRAY_SIZE : (i + 1) * chunk;
        pthread_create(&threads[i], NULL, compute_stats, &args[i]);
    }

    long total_sum = 0;
    int  global_min = INT_MAX, global_max = INT_MIN;

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        total_sum  += args[i].partial_sum;
        if (args[i].partial_min < global_min) global_min = args[i].partial_min;
        if (args[i].partial_max > global_max) global_max = args[i].partial_max;
        printf("  Thread %d: sum=%ld  min=%d  max=%d\n",
               i, args[i].partial_sum, args[i].partial_min, args[i].partial_max);
    }
    printf("Final: sum=%ld (expected %d)  min=%d  max=%d\n\n",
           total_sum, ARRAY_SIZE, global_min, global_max);

    /* ---- Approach B ------------------------------------------- */
    printf("=== Approach B: heap-allocated return value ===\n");

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].array = array;
        args[i].start = i * chunk;
        args[i].end   = (i == NUM_THREADS - 1) ? ARRAY_SIZE : (i + 1) * chunk;
        pthread_create(&threads[i], NULL, sum_only, &args[i]);
    }

    long total_sum_b = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        heap_result_t *r = NULL;
        pthread_join(threads[i], (void **)&r);   /* capture return value */
        printf("  Thread %d: sum=%ld\n", i, r->sum);
        total_sum_b += r->sum;
        free(r);   /* caller's responsibility */
    }
    printf("Final: sum=%ld (expected %d)\n", total_sum_b, ARRAY_SIZE);

    free(array);
    return 0;
}
