/*
 * 04_readers_writers.c
 *
 * The Readers-Writers problem: a shared "database" (an integer array)
 * that many threads read and occasionally write.
 *
 * Rules:
 *   - Any number of readers may read SIMULTANEOUSLY (they don't modify data)
 *   - A writer needs EXCLUSIVE access — no readers or other writers
 *
 * A plain mutex is too restrictive: it would prevent concurrent reads,
 * even though they're perfectly safe. pthread_rwlock_t solves this.
 *
 *   pthread_rwlock_rdlock(&rwlock)  — shared read lock (many threads at once)
 *   pthread_rwlock_wrlock(&rwlock)  — exclusive write lock (one thread only)
 *   pthread_rwlock_unlock(&rwlock)  — release either kind
 *
 * This demo runs 6 reader threads and 2 writer threads simultaneously.
 * Each reader acquires a read lock, reads a value, and releases.
 * Each writer acquires a write lock, increments a value, and releases.
 * The timestamp output shows that multiple readers run concurrently
 * while writers run alone.
 *
 * Compile:
 *   gcc -Wall -pthread -std=c11 04_readers_writers.c -o readers_writers
 *
 * Run:
 *   ./readers_writers
 *
 * Look for readers with overlapping time ranges in the output. Any two
 * lines from different readers where the read-end of one overlaps the
 * read-start of another confirm concurrent reads happened. You will
 * NEVER see a reader and a writer overlap — the rwlock prevents it.
 */

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define DB_SIZE        10
#define NUM_READERS     6
#define NUM_WRITERS     2
#define OPS_PER_READER  5
#define OPS_PER_WRITER  3

/* Shared "database" */
int db[DB_SIZE];
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

/* Tiny timestamp helper: milliseconds since program start */
static struct timespec prog_start;
static long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec  - prog_start.tv_sec)  * 1000
         + (ts.tv_nsec - prog_start.tv_nsec) / 1000000;
}

void *reader(void *arg) {
    int id = *(int *)arg;
    for (int op = 0; op < OPS_PER_READER; op++) {
        usleep(rand() % 80000 + 20000);     /* think for 20-100 ms */

        long t_start = now_ms();
        pthread_rwlock_rdlock(&rwlock);     /* ACQUIRE READ LOCK */

        int val = db[id % DB_SIZE];         /* read a value */
        usleep(50000);                      /* simulate reading time */

        pthread_rwlock_unlock(&rwlock);     /* RELEASE */
        long t_end = now_ms();

        printf("[reader %d] read db[%d]=%d  (held lock %ldms, t=%ld-%ld)\n",
               id, id % DB_SIZE, val, t_end - t_start, t_start, t_end);
    }
    return NULL;
}

void *writer(void *arg) {
    int id = *(int *)arg;
    for (int op = 0; op < OPS_PER_WRITER; op++) {
        usleep(rand() % 150000 + 100000);   /* think longer than readers */

        long t_start = now_ms();
        pthread_rwlock_wrlock(&rwlock);     /* ACQUIRE WRITE LOCK (exclusive) */

        db[id % DB_SIZE]++;                 /* modify a value */
        usleep(80000);                      /* simulate writing time */

        pthread_rwlock_unlock(&rwlock);     /* RELEASE */
        long t_end = now_ms();

        printf("[WRITER %d] wrote db[%d]=%d  (held lock %ldms, t=%ld-%ld) ***\n",
               id, id % DB_SIZE, db[id % DB_SIZE], t_end - t_start, t_start, t_end);
    }
    return NULL;
}

int main(void) {
    clock_gettime(CLOCK_MONOTONIC, &prog_start);
    srand(42);

    /* Initialize database */
    for (int i = 0; i < DB_SIZE; i++)
        db[i] = i * 10;

    pthread_t readers[NUM_READERS];
    pthread_t writers[NUM_WRITERS];
    int       r_ids[NUM_READERS];
    int       w_ids[NUM_WRITERS];

    printf("Starting %d readers and %d writers.\n", NUM_READERS, NUM_WRITERS);
    printf("Watch for readers with overlapping t= ranges — those ran in parallel.\n");
    printf("Writers (marked ***) always appear exclusively.\n\n");

    for (int i = 0; i < NUM_READERS; i++) {
        r_ids[i] = i;
        pthread_create(&readers[i], NULL, reader, &r_ids[i]);
    }
    for (int i = 0; i < NUM_WRITERS; i++) {
        w_ids[i] = i;
        pthread_create(&writers[i], NULL, writer, &w_ids[i]);
    }

    for (int i = 0; i < NUM_READERS; i++) pthread_join(readers[i], NULL);
    for (int i = 0; i < NUM_WRITERS; i++) pthread_join(writers[i], NULL);

    printf("\nFinal database state:\n");
    for (int i = 0; i < DB_SIZE; i++)
        printf("  db[%d] = %d\n", i, db[i]);

    pthread_rwlock_destroy(&rwlock);
    return 0;
}
