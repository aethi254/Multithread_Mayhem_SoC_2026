// 02_worked_example_sched_print.md
//
// Worked Example: Adding a "schedinfo" syscall to print the
// current process's scheduling state.
//
// This walks through adding a syscall that reads kernel
// scheduling state (the current process's priority, state,
// and how many times it has been scheduled) and returns it
// to user space.
//
// Do this in your own xv6-riscv checkout and build/test it
// before starting Problem 2. The syscalls in Problems 2-4
// are variations on this same five-step pattern.

---

## Overview

We will add a syscall `schedinfo()` that takes no arguments
and returns a struct containing:
- The calling process's current state (as an int)
- The calling process's priority (an int, default 31)
- How many times this process has been scheduled to run

This requires:
1. A new field `sched_count` in `struct proc`
2. Incrementing it in `scheduler()` when a process is chosen
3. The usual five touch points for the syscall

---

## Step 1: `kernel/proc.h` — add fields

```c
struct proc {
    ...
    int priority;       // scheduling priority (0=highest, 63=lowest)
    int sched_count;    // number of times scheduled
    ...
};
```

---

## Step 2: `kernel/proc.c` — initialize fields in allocproc

In `allocproc()`, after the existing field initializations:

```c
p->priority = 31;     // default mid-range priority
p->sched_count = 0;
```

---

## Step 3: `kernel/proc.c` — increment sched_count in scheduler

Inside `scheduler()`, right before the context switch:

```c
if (p->state == RUNNABLE) {
    p->state = RUNNING;
    p->sched_count++;   // count this scheduling event
    c->proc = p;
    swtch(&c->context, &p->context);
    ...
}
```

---

## Step 4: `kernel/syscall.h` — pick a number

```c
#define SYS_schedinfo   22   // one past the highest existing
```

---

## Step 5: `user/user.h` — declare for user programs

```c
// Structured return for schedinfo
struct sched_info {
    int state;
    int priority;
    int sched_count;
};

int schedinfo(struct sched_info *info);
```

Notice this syscall takes a **pointer** argument — the user
program allocates a struct on its stack and passes a pointer
for the kernel to fill in using `copyout`.

---

## Step 6: `user/usys.pl` — generate the stub

```perl
entry("schedinfo");
```

---

## Step 7: `kernel/syscall.c` — wire it in

```c
extern uint64 sys_schedinfo(void);

...

[SYS_schedinfo] sys_schedinfo,
```

---

## Step 8: `kernel/sysproc.c` — the implementation

```c
uint64
sys_schedinfo(void)
{
    uint64 addr;
    struct sched_info info;
    struct proc *p = myproc();

    if (argaddr(0, &addr) < 0)
        return -1;

    info.state = p->state;
    info.priority = p->priority;
    info.sched_count = p->sched_count;

    if (copyout(p->pagetable, addr, (char *)&info, sizeof(info)) < 0)
        return -1;

    return 0;
}
```

---

## Build and test

```bash
cd xv6-riscv
make clean && make && make qemu
```

Write `user/schedinfo_test.c`:

```c
#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
    struct sched_info info;

    if (schedinfo(&info) < 0) {
        printf("schedinfo failed\n");
        exit(1);
    }

    printf("State:       %d\n", info.state);
    printf("Priority:    %d\n", info.priority);
    printf("Sched count: %d\n", info.sched_count);

    exit(0);
}
```

Add to `UPROGS`, rebuild, and test:

```
$ schedinfo_test
State:       3
Priority:    31
Sched count: 1
```

The state value `3` corresponds to `RUNNING` (check the
`enum procstate` in `kernel/proc.h`). The sched_count will
be at least `1` — the process was scheduled once to start,
and if it calls other syscalls that sleep, it may have been
scheduled multiple times.

---

## What you've learned

- How to add a kernel data field and wire it through the scheduler
- How to use `copyout` (from Week 5) with a struct
- The five touch points, applied to a scheduling-aware syscall

This pattern — add a field, initialize it, update it in the
scheduler, expose it via a syscall — is exactly what you'll
do for Problems 2-4.
