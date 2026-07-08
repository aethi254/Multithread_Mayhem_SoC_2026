// 01_sched_trace.c
//
// Example: observe scheduling behavior from user space.
// This program forks several child processes and uses uptime()
// to measure how much CPU time each one gets under xv6's
// round-robin scheduler.
//
// Run this and observe whether all children get roughly equal
// time. Then read kernel/proc.c's scheduler() loop and think
// about why a given child might get more or less than its
// fair share on any particular run.
//
// This program does NOT require kernel changes. It runs on a
// stock xv6.

#include "kernel/types.h"
#include "user/user.h"

#define NCHILD 4
#define DURATION 50   // ticks to run (approx 0.5s at xv6's default tick)

int
main(void)
{
  int pid;
  int children[NCHILD];
  int pipefds[NCHILD][2];
  int start, end;

  printf("=== Scheduler Trace Demo ===\n");
  printf("Forking %d children, running for %d ticks...\n\n", NCHILD, DURATION);

  start = uptime();

  for (int i = 0; i < NCHILD; i++) {
    if (pipe(pipefds[i]) < 0) {
      printf("pipe failed\n");
      exit(1);
    }

    pid = fork();
    if (pid < 0) {
      printf("fork failed\n");
      exit(1);
    }

    if (pid == 0) {
      // Child process
      close(pipefds[i][0]);   // close read end
      volatile uint64 count = 0;

      // Busy-wait until DURATION ticks have elapsed
      int deadline = start + DURATION;
      while (uptime() < deadline) {
        count++;
        // Prevent the compiler from optimizing the loop away
        __sync_synchronize();
      }

      // Write count back to parent via pipe
      write(pipefds[i][1], (char *)&count, sizeof(count));
      close(pipefds[i][1]);
      exit(0);
    }

    // Parent: record child PID and close write end
    children[i] = pid;
    close(pipefds[i][1]);
  }

  // Parent waits for all children
  for (int i = 0; i < NCHILD; i++) {
    uint64 ccount = 0;
    int bytes = read(pipefds[i][0], (char *)&ccount, sizeof(ccount));
    if (bytes == sizeof(ccount)) {
      printf("Child %d (PID %d): %lu iterations\n", i, children[i], ccount);
    }
    close(pipefds[i][0]);
    wait(0);
  }

  end = uptime();
  printf("\nTotal ticks elapsed: %d\n", end - start);

  printf("\nObservation: under xv6's round-robin scheduler, all children\n");
  printf("should get roughly equal CPU time when run simultaneously.\n");
  printf("If one child consistently gets more, think about whether\n");
  printf("the scheduler's proc[] scan order could be causing bias.\n");

  exit(0);
}
