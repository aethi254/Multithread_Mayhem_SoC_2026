/*
 * 02_race_counter.c
 *
 * The classic race condition demo.
 *
 * Two threads each increment a SHARED counter ITERS times.
 * If everything went well, the final value would be 2 * ITERS.
 * It usually won't be.
 *
 * Why? Because counter++ is not one operation. It's three:
 *     1. load counter from memory into a register
 *     2. add 1 to that register
 *     3. store the register's value back to memory
 *
 * Two threads running these three steps concurrently can interleave
 * such that both load the same value, both compute the same +1, and
 * both store back — effectively turning two increments into one.
 *
 * Compile:
 *   gcc -Wall -pthread 02_race_counter.c -o race_counter
 *
 * Run multiple times:
 *   ./race_counter
 *   ./race_counter
 *   ./race_counter
 *
 * Try also:
 *   - Increasing ITERS (e.g., to 100,000,000). The error gets bigger.
 *   - Compiling with optimization: gcc -O2 -pthread ... — different
 *     surprises can appear.
 */

#include <stdio.h>
#include <pthread.h>

#define ITERS 10000000

/* A SHARED global counter. Both threads will modify this. */
long counter = 0;

void *increment(void *arg) {
    (void)arg;         /* unused — pthread signature requires it */
    for (long i = 0; i < ITERS; i++) {
        counter++;     /* three machine instructions in disguise */
    }
    return NULL;
}

int main(void) {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, increment, NULL);
    pthread_create(&t2, NULL, increment, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    long expected = 2L * ITERS;
    printf("Final counter:  %ld\n", counter);
    printf("Expected:       %ld\n", expected);
    printf("Difference:     %ld\n", expected - counter);

    if (counter == expected) {
        printf("Result: CORRECT (but try running it again, you might get unlucky)\n");
    } else {
        printf("Result: WRONG by %ld — race condition observed.\n", expected - counter);
    }

    return 0;
}
