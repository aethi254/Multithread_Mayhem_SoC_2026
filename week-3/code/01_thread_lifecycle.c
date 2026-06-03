/*
 * 01_thread_lifecycle.c
 *
 * Demonstrates the two key lifecycle choices you make when creating
 * a thread: joinable (the default) vs detached.
 *
 * JOINABLE threads (Part 1):
 *   - The OS keeps the thread's exit status after it terminates.
 *   - You MUST call pthread_join to retrieve the status and free OS
 *     resources. Not joining is a resource leak.
 *   - Good when: you need to wait for the thread, or collect its
 *     return value.
 *
 * DETACHED threads (Part 2):
 *   - The OS cleans up the thread's resources automatically when it
 *     terminates. You cannot join a detached thread.
 *   - Good when: fire-and-forget (server connection handlers, background
 *     loggers, etc.).
 *   - Two ways to detach: pthread_attr_setdetachstate before creation,
 *     or pthread_detach after creation.
 *
 * Compile:
 *   gcc -Wall -pthread -std=c11 01_thread_lifecycle.c -o thread_lifecycle
 *
 * Run:
 *   ./thread_lifecycle
 *
 * Observe: joinable threads print in creation order (we force this by
 * joining in order). Detached threads may print in any order.
 */

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define NUM_JOINABLE  4
#define NUM_DETACHED  4

/* ------------------------------------------------------------------ */
/* PART 1: JOINABLE THREADS                                             */
/* ------------------------------------------------------------------ */

void *joinable_worker(void *arg) {
    int id = *(int *)arg;
    printf("[joinable %d] running on thread ID %lu\n",
           id, (unsigned long)pthread_self());
    usleep(100000 * id);      /* stagger so output order is visible */
    printf("[joinable %d] done\n", id);
    return NULL;
}

/* ------------------------------------------------------------------ */
/* PART 2: DETACHED THREADS                                             */
/* ------------------------------------------------------------------ */

/*
 * This semaphore-like counter lets main know when all detached threads
 * have finished. We can't use pthread_join on detached threads, so we
 * need another way to wait for them.
 */
volatile int detached_done = 0;
pthread_mutex_t done_lock   = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  done_cond   = PTHREAD_COND_INITIALIZER;

void *detached_worker(void *arg) {
    int id = *(int *)arg;
    free(arg);                /* we malloc'd this — thread owns and frees it */
    printf("[detached %d] running\n", id);
    usleep(80000 * (id + 1));
    printf("[detached %d] done\n", id);

    pthread_mutex_lock(&done_lock);
    detached_done++;
    pthread_cond_signal(&done_cond);
    pthread_mutex_unlock(&done_lock);
    return NULL;
}

int main(void) {
    /* ---- Part 1: Joinable threads --------------------------------- */
    printf("=== Part 1: Joinable threads ===\n");

    pthread_t joinable[NUM_JOINABLE];
    int ids[NUM_JOINABLE];

    for (int i = 0; i < NUM_JOINABLE; i++) {
        ids[i] = i;
        /* No attributes needed — joinable is the default. */
        pthread_create(&joinable[i], NULL, joinable_worker, &ids[i]);
    }

    for (int i = 0; i < NUM_JOINABLE; i++) {
        pthread_join(joinable[i], NULL);
        printf("[main] joined thread %d\n", i);
    }

    /* ---- Part 2: Detached threads --------------------------------- */
    printf("\n=== Part 2: Detached threads ===\n");

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    for (int i = 0; i < NUM_DETACHED; i++) {
        /*
         * We malloc each ID because all threads share the loop scope.
         * The thread frees its own ID when done. Stack allocation (&i)
         * would be a race condition.
         */
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_t t;
        pthread_create(&t, NULL, detached_worker, id);
        /* We do not store 't' — there's nothing to join. */
    }

    pthread_attr_destroy(&attr);

    /* Wait for all detached threads to finish. */
    pthread_mutex_lock(&done_lock);
    while (detached_done < NUM_DETACHED)
        pthread_cond_wait(&done_cond, &done_lock);
    pthread_mutex_unlock(&done_lock);

    printf("[main] all detached threads confirmed done\n");

    /* ---- Part 3: pthread_detach after creation -------------------- */
    printf("\n=== Part 3: pthread_detach after creation ===\n");
    {
        int *id = malloc(sizeof(int));
        *id = 99;
        pthread_t t;
        pthread_create(&t, NULL, detached_worker, id);
        pthread_detach(t);    /* detach AFTER creation — same effect */

        pthread_mutex_lock(&done_lock);
        while (detached_done < NUM_DETACHED + 1)
            pthread_cond_wait(&done_cond, &done_lock);
        pthread_mutex_unlock(&done_lock);

        printf("[main] post-creation detach example done\n");
    }

    printf("\nAll done. Note: for joinable threads, 'done' prints BEFORE\n");
    printf("'joined' because the join happens after the thread exits.\n");

    pthread_mutex_destroy(&done_lock);
    pthread_cond_destroy(&done_cond);
    return 0;
}
