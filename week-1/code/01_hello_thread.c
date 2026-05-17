/*
 * 01_hello_thread.c
 *
 * The simplest possible multithreaded program.
 *
 * Creates NUM_THREADS threads, each of which prints a hello message
 * with its assigned ID, then exits. The main thread waits for all of
 * them with pthread_join.
 *
 * Goal of this example:
 *   - See pthread_create and pthread_join in action.
 *   - Notice that the order of the printed messages is NOT deterministic.
 *   - Run the program multiple times. The output order will likely change.
 *
 * Compile:
 *   gcc -Wall -pthread 01_hello_thread.c -o hello_thread
 *
 * Run several times and observe the ordering:
 *   ./hello_thread
 *   ./hello_thread
 *   ./hello_thread
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 5

/*
 * The function each thread will run.
 *
 * pthread requires the function signature to be:
 *     void *function_name(void *arg);
 *
 * We pass each thread a pointer to its integer ID, which we cast back
 * to int * inside the function.
 */
void *say_hello(void *arg) {
    int id = *(int *)arg;
    printf("Hello from thread %d\n", id);
    return NULL;     /* we have nothing to return */
}

int main(void) {
    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];

    /*
     * IMPORTANT: we store each ID in its own slot of the ids[] array
     * and pass a pointer to that slot. If we passed &i instead, all
     * threads would share the same address and read whatever value of
     * i happened to be there when they got around to looking — usually
     * NUM_THREADS, since the loop finishes fast.
     */
    for (int i = 0; i < NUM_THREADS; i++) {
        ids[i] = i;
        int rc = pthread_create(&threads[i], NULL, say_hello, &ids[i]);
        if (rc != 0) {
            fprintf(stderr, "pthread_create failed for thread %d\n", i);
            exit(1);
        }
    }

    /*
     * Wait for every thread to finish before letting main return.
     * If main returns while threads are still running, the whole
     * process is torn down and those threads die mid-execution.
     */
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Main thread: all done.\n");
    return 0;
}
