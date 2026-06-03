/*
 * 03_semaphores.c
 *
 * Introduction to POSIX semaphores (sem_t) and how they produce a
 * cleaner formulation of the producer-consumer problem than the
 * mutex + condition-variable approach from Week 2.
 *
 * A semaphore is an integer counter with two atomic operations:
 *   sem_wait (P): if counter > 0, decrement and return; else BLOCK
 *   sem_post (V): increment the counter; if threads are waiting, wake one
 *
 * PART 1 — Ordering guarantee (the simplest semaphore use-case):
 *   Thread B must not run until Thread A finishes its work. Initialize
 *   a semaphore to 0; A posts it after finishing; B waits on it.
 *   This is cleaner than a condvar because no mutex is needed.
 *
 * PART 2 — Producer-consumer with semaphores:
 *   Two semaphores replace the two condition variables from Week 2:
 *     'empty' = available slots (initialized to BUFFER_SIZE)
 *     'full'  = available items (initialized to 0)
 *   Producer does: sem_wait(empty) → put item → sem_post(full)
 *   Consumer does: sem_wait(full)  → take item → sem_post(empty)
 *   A mutex still protects the buffer itself, but the wait logic
 *   is handled entirely by the semaphores — no while-loop needed.
 *
 * Compile (Linux):
 *   gcc -Wall -pthread -std=c11 03_semaphores.c -o semaphores
 *
 * Note: sem_init (unnamed semaphores) is not available on macOS.
 * On macOS, use sem_open (named semaphores) or Grand Central Dispatch.
 *
 * Run:
 *   ./semaphores
 */

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

/* ================================================================== */
/* PART 1: Ordering guarantee                                           */
/* ================================================================== */
sem_t order_sem;

void *part1_a(void *arg) {
    (void)arg;
    printf("[A] doing work...\n");
    usleep(300000);
    printf("[A] work done — signalling B\n");
    sem_post(&order_sem);   /* signal B it can proceed */
    return NULL;
}

void *part1_b(void *arg) {
    (void)arg;
    printf("[B] waiting for A to finish...\n");
    sem_wait(&order_sem);   /* block until A posts */
    printf("[B] A is done, now I can proceed\n");
    return NULL;
}

/* ================================================================== */
/* PART 2: Producer-consumer with semaphores                            */
/* ================================================================== */
#define BUFFER_SIZE       5
#define ITEMS_TO_PRODUCE  20

int  buffer[BUFFER_SIZE];
int  in = 0, out = 0;

sem_t           empty;       /* counts free slots */
sem_t           full;        /* counts available items */
pthread_mutex_t buf_lock = PTHREAD_MUTEX_INITIALIZER;

void *producer(void *arg) {
    (void)arg;
    for (int i = 0; i < ITEMS_TO_PRODUCE; i++) {
        sem_wait(&empty);             /* wait for a free slot */
        pthread_mutex_lock(&buf_lock);

        buffer[in] = i;
        in = (in + 1) % BUFFER_SIZE;

        int cur;
        sem_getvalue(&full, &cur);
        printf("[producer] put %2d  (items in buffer: %d)\n", i, cur + 1);

        pthread_mutex_unlock(&buf_lock);
        sem_post(&full);              /* signal that a new item is ready */
        usleep(40000);
    }
    return NULL;
}

void *consumer(void *arg) {
    (void)arg;
    for (int i = 0; i < ITEMS_TO_PRODUCE; i++) {
        sem_wait(&full);              /* wait for an item */
        pthread_mutex_lock(&buf_lock);

        int item = buffer[out];
        out = (out + 1) % BUFFER_SIZE;

        int cur;
        sem_getvalue(&empty, &cur);
        printf("[consumer] got %2d  (free slots: %d)\n", item, cur + 1);

        pthread_mutex_unlock(&buf_lock);
        sem_post(&empty);             /* free the slot */
        usleep(100000);               /* consumer is slower */
    }
    return NULL;
}

int main(void) {
    /* ---- Part 1 -------------------------------------------------- */
    printf("=== Part 1: Ordering guarantee with a semaphore ===\n\n");

    sem_init(&order_sem, 0, 0);   /* start at 0 → B blocks until A posts */

    pthread_t a, b;
    pthread_create(&b, NULL, part1_b, NULL);
    usleep(50000);                /* give B a head start so we see it wait */
    pthread_create(&a, NULL, part1_a, NULL);

    pthread_join(a, NULL);
    pthread_join(b, NULL);
    sem_destroy(&order_sem);

    /* ---- Part 2 -------------------------------------------------- */
    printf("\n=== Part 2: Producer-consumer with semaphores ===\n\n");

    sem_init(&empty, 0, BUFFER_SIZE);   /* all slots start free */
    sem_init(&full,  0, 0);             /* no items yet */

    pthread_t p, c;
    pthread_create(&p, NULL, producer, NULL);
    pthread_create(&c, NULL, consumer, NULL);
    pthread_join(p, NULL);
    pthread_join(c, NULL);

    printf("\nAll %d items produced and consumed — no items lost.\n",
           ITEMS_TO_PRODUCE);
    printf("Notice: no while-loop around sem_wait. The count IS the condition.\n");

    sem_destroy(&empty);
    sem_destroy(&full);
    pthread_mutex_destroy(&buf_lock);
    return 0;
}
