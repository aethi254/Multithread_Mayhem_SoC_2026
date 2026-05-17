/*
 * 04_bank_chaos.c
 *
 * A more realistic race condition that looks like a real-world bug.
 *
 * We simulate a shared bank account with a starting balance of $10000.
 * Two threads represent two customers, each of whom tries to make
 * many small withdrawals. Each withdrawal is guarded by a balance
 * check: only proceed if there's enough money.
 *
 * The check-then-act pattern is the classic source of the
 * "how did the account go negative?" bug. The check and the act
 * are two separate steps, and another thread can do its own
 * withdrawal in between them.
 *
 * Goal of this example:
 *   - See a race condition in code that looks more like real
 *     business logic, not a toy counter.
 *   - Watch the balance possibly go negative, or watch the
 *     successful-counts disagree with the balance.
 *
 * Compile:
 *   gcc -Wall -pthread 04_bank_chaos.c -o bank_chaos
 *
 * Run several times:
 *   ./bank_chaos
 *
 * You'll often see a final balance that's negative — which a bank
 * would consider a serious problem.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int balance = 10000;     /* SHARED account balance, in dollars */

#define ATTEMPTS_PER_CUSTOMER 100000
#define WITHDRAWAL_AMOUNT     1

void *customer(void *arg) {
    const char *name = (const char *)arg;
    int successful = 0;
    int rejected   = 0;

    for (int i = 0; i < ATTEMPTS_PER_CUSTOMER; i++) {
        /*
         * Classic check-then-act. Looks safe. Isn't.
         * Between the if and the subtraction, the other thread can
         * run its own if-and-subtraction, and both can succeed even
         * when there was only enough money for one.
         */
        if (balance >= WITHDRAWAL_AMOUNT) {
            balance -= WITHDRAWAL_AMOUNT;
            successful++;
        } else {
            rejected++;
        }
    }

    printf("Customer %s: %d successful, %d rejected\n",
           name, successful, rejected);
    return NULL;
}

int main(void) {
    pthread_t alice, bob;

    pthread_create(&alice, NULL, customer, (void *)"Alice");
    pthread_create(&bob,   NULL, customer, (void *)"Bob");

    pthread_join(alice, NULL);
    pthread_join(bob,   NULL);

    printf("Final balance: %d\n", balance);

    if (balance < 0) {
        printf("The account went NEGATIVE. The bank is now suing somebody.\n");
    } else if (balance > 0) {
        printf("Some money is left over. That should not be possible if both\n");
        printf("customers were determined to drain it — unless something\n");
        printf("went wrong with the accounting.\n");
    } else {
        printf("Balance is exactly zero — but check the success counts above:\n");
        printf("do successful_alice + successful_bob = 10000? If not, race.\n");
    }

    return 0;
}
