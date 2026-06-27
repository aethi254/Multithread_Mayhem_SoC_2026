// 02_self_timing_demo.c
//
// Example: a system call is not "free" the way calling an ordinary
// local C function is - every single one of these getpid() calls
// crosses into the kernel and back via the full trap path described
// in theory.md (uservec -> usertrap -> syscall -> usertrapret -> userret).
//
// This program calls getpid() a large number of times in a loop and
// times the whole loop using uptime() (ticks since boot). It will NOT
// give you a precise per-call cost - xv6's clock tick is coarse and
// this is a teaching demo, not a benchmark. The point is just to see
// a *nonzero*, *measurable* number of ticks pass for work that does
// "nothing" from the program's point of view.
//
// A NOTE ON PRECISION: if you see 0 ticks elapsed, that just means the
// loop finished within a single tick interval on your build. Increase
// ITERS and try again. Mention what you observed in your report either
// way - "I couldn't measure a difference" is a valid, honest result.

#include "kernel/types.h"
#include "user/user.h"

#define ITERS 2000000

int
main(void)
{
  printf("=== Self-Timing Demo ===\n");
  printf("Calling getpid() %d times...\n", ITERS);

  int start = uptime();

  for (int i = 0; i < ITERS; i++) {
    getpid();   // result discarded - we only care about the crossing
  }

  int end = uptime();

  printf("Ticks elapsed: %d\n", end - start);
  printf("(Each call still crossed into the kernel and back - see\n");
  printf(" theory.md Section 2 for exactly what happened %d times.)\n",
         ITERS);

  exit(0);
}
