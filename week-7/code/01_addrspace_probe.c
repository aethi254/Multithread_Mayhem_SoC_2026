// 01_addrspace_probe.c
//
// Example: build intuition for where a thread's stack would have to go.
//
// This program runs on STOCK xv6 - no kernel changes required. It just
// calls sbrk() a few times and prints the returned addresses, so you can
// see concretely that "the top of the address space" is a real, moving
// number you can observe from user space - not an abstract idea.
//
// sbrk(n) grows the calling process's heap by n bytes and returns the
// PREVIOUS break (i.e., the address right before the newly mapped
// region starts). Under the hood this calls growproc() -> uvmalloc(),
// the exact same function Problem 3's alloc_thread_stack() calls this
// week - the only difference is that sbrk() is meant for heap growth
// and Problem 3 repurposes the same mechanism for stack allocation.
//
// Read this before starting Problem 3: alloc_thread_stack() is doing
// nothing more exotic than what sbrk() already does every day.
//
// NOTE ON PRINTF: xv6's printf only understands %d, %x, %s, %c, and %lu
// (for a 64-bit unsigned value) - there is no %p. Addresses below are
// therefore cast to (uint64) and printed with %lu, same convention used
// in earlier weeks' code.

#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  printf("=== Address Space Probe ===\n\n");

  char *initial_break = sbrk(0);
  printf("Current break (top of mapped memory): %lu\n",
         (uint64)initial_break);

  printf("\nGrowing the address space by 4096 bytes (one page)...\n");
  char *before1 = sbrk(4096);
  char *after1 = sbrk(0);
  printf("sbrk(4096) returned (old break): %lu\n", (uint64)before1);
  printf("New break:                       %lu\n", (uint64)after1);
  printf("Growth observed:                 %lu bytes\n",
         (uint64)(after1 - before1));

  printf("\nGrowing again by 8192 bytes (two pages)...\n");
  char *before2 = sbrk(8192);
  char *after2 = sbrk(0);
  printf("sbrk(8192) returned (old break): %lu\n", (uint64)before2);
  printf("New break:                       %lu\n", (uint64)after2);
  printf("Growth observed:                 %lu bytes\n",
         (uint64)(after2 - before2));

  printf("\nWriting to the first byte of the newly mapped region...\n");
  *before1 = 'X';
  printf("Wrote 'X' at address %lu successfully - it's real, mapped memory.\n",
         (uint64)before1);

  printf("\nObservation: every one of these addresses is inside the SAME\n");
  printf("address space, growing upward from wherever the process's\n");
  printf("code, data, and original stack left off. This week's\n");
  printf("alloc_thread_stack() (Problem 3) grows the address space in\n");
  printf("exactly this same direction, for exactly the same underlying\n");
  printf("reason - it just calls the memory 'a thread's stack' instead\n");
  printf("of 'the heap'. The kernel doesn't know or care about that\n");
  printf("distinction; it's purely a convention on the user-space side.\n");

  exit(0);
}
