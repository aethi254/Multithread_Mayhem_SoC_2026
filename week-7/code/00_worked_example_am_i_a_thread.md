# Worked Example: Adding `am_i_a_thread()` — a syscall reading one field

This walks through adding one complete, trivial system call from scratch, touching every one of the five places listed in Week 5's theory Section 5. `am_i_a_thread()` takes no arguments and returns the calling process's `is_thread` field (which, this week, is always `0` for every process — nothing sets it to `1` yet, since that's Week 8's job). It exists purely so you see the *thread-bookkeeping* fields wired through the syscall path once, end to end, with the smallest possible example, before you build the fuller `getthreadinfo()` in Problem 2.

**Do this in your own `xv6-riscv` checkout, and build/test it, before starting the problem set.** Problem 2 is a different, slightly larger syscall (returning a struct instead of a single int) — don't just copy these diffs.

---

## 1. `kernel/proc.h` — add the field

Find `struct proc` and add one new field. This is the first of the two fields theory Section 3 describes; `tgid` follows the same pattern and is left to Problem 2.

```c
struct proc {
  struct spinlock lock;
  ...
  char name[16];
  int is_thread;        // <-- new: 0 = ordinary process (always 0 this week)
};
```

---

## 2. `kernel/proc.c` — initialize it in `allocproc()`

Find `allocproc()` and, near where the other simple integer fields are reset for a freshly allocated slot, add:

```c
p->is_thread = 0;
```

Every process, without exception, starts life with `is_thread == 0`. Nothing in this worked example ever sets it to anything else — that's deliberate, and it's exactly what Week 8's `thread_create()` will change.

---

## 3. `kernel/syscall.h` — pick a number

```c
#define SYS_close        21
#define SYS_am_i_a_thread 22   // <-- new
```

If you've already added a syscall from an earlier week, use the next number after that one instead.

---

## 4. `user/user.h` — declare it for user programs

```c
int am_i_a_thread(void);
```

---

## 5. `user/usys.pl` — generate the user-space stub

```perl
entry("close");
entry("am_i_a_thread");   # <-- new
```

After your next `make`, `user/usys.S` will contain a freshly generated stub — you don't write this by hand:

```asm
.global am_i_a_thread
am_i_a_thread:
 li a7, SYS_am_i_a_thread
 ecall
 ret
```

---

## 6. `kernel/syscall.c` — wire it into the dispatch table

```c
extern uint64 sys_close(void);
extern uint64 sys_am_i_a_thread(void);   // <-- new
```

```c
static uint64 (*syscalls[])(void) = {
[SYS_close]         sys_close,
[SYS_am_i_a_thread] sys_am_i_a_thread,   // <-- new
};
```

---

## 7. `kernel/sysproc.c` — the actual implementation

```c
uint64
sys_am_i_a_thread(void)
{
  struct proc *p = myproc();
  return p->is_thread;
}
```

No arguments to extract, so no `argint`/`argaddr`/`argstr` calls needed — same shape as `sys_getpid()`.

---

## Build and test it

```bash
cd xv6-riscv
make clean && make && make qemu
```

Write a tiny test program, `user/am_i_a_thread_test.c`:

```c
#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int result = am_i_a_thread();
  printf("am_i_a_thread() returned %d\n", result);
  if (result == 0) {
    printf("Correct: every process this week is an ordinary process,\n");
    printf("not a thread. Week 8's thread_create() is what will make\n");
    printf("this return 1 for a thread's struct proc.\n");
  }
  exit(0);
}
```

Add it to `UPROGS` in the Makefile (`$U/_am_i_a_thread_test\`), rebuild, and run it inside QEMU:

```
$ am_i_a_thread_test
am_i_a_thread() returned 0
Correct: every process this week is an ordinary process,
not a thread. Week 8's thread_create() is what will make
this return 1 for a thread's struct proc.
```

If you see `0`, the field is wired through correctly. Problem 2 asks you to add the second field (`tgid`) and expose both of them, plus your own `pid`, through a single struct-returning syscall — the exact same five-step pattern, just with `copyout` instead of a bare `int` return.
