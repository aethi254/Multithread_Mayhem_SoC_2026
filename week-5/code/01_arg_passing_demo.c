// 01_arg_passing_demo.c
//
// Example: the same "function call" syntax hides three different
// argument shapes the kernel has to extract from the trapframe:
//   - no arguments at all       (getpid)
//   - a plain integer           (sleep)
//   - a pointer + a length      (write)
//
// Run this, then go read argint()/argaddr() in kernel/syscall.c and
// match each call below to how its arguments would be fetched on the
// kernel side. This sets up theory.md Section 4.

#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  printf("=== Argument Passing Demo ===\n\n");

  // Shape 1: no arguments. The kernel side just calls sys_getpid()
  // with nothing to extract from a0-a5 at all.
  int pid = getpid();
  printf("getpid()            -> %d   (zero arguments)\n", pid);

  // Shape 2: a single plain integer. The kernel side calls
  // argint(0, &n) to pull it out of a0.
  printf("sleep(5)            -> ");
  int slept = sleep(5);
  printf("%d   (one int argument, ticks to sleep)\n", slept);

  // Shape 3: a pointer and a length. The kernel side calls
  // argaddr(1, &addr) for the buffer and argint(2, &n) for the length,
  // then uses the address with copyin/copyout — never dereferenced
  // directly. (fd is argument 0, a plain int, same as sleep's argument.)
  char msg[] = "this string is the buffer argument\n";
  int n = write(1, msg, sizeof(msg) - 1);
  printf("write(1, buf, %2d)   -> %d   (int + pointer + int arguments)\n",
         (int)sizeof(msg) - 1, n);

  printf("\nDone. Notice all three calls *look* like ordinary C function\n");
  printf("calls from here — the trapframe machinery is invisible from\n");
  printf("user space. That's the whole point of the syscall stubs.\n");

  exit(0);
}
