# Week 6 Theory: Processes & Scheduling

> Last week you learned how user programs cross into the kernel via system calls. This week we open up the kernel's inner loop — the scheduler — and look at how xv6 decides which process gets the CPU, how it switches between them, and how spinlocks make all of this safe.

---

## 1. The Process Control Block (PCB) — `struct proc`

Every process in xv6 is represented by a `struct proc` in `kernel/proc.h`. You saw this in Week 4 as a reference; this week we care about the fields that scheduling touches:

```c
struct proc {
    struct spinlock lock;
    enum procstate state;   // UNUSED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE
    void *chan;             // sleep channel (if SLEEPING)
    int pid;
    struct proc *parent;
    pagetable_t pagetable;
    uint64 kstack;          // kernel stack address
    uint64 sz;
    struct trapframe *trapframe;
    struct context context; // saved registers for context switch
    struct file *ofile[NOFILE];
    struct inode *cwd;
    char name[16];
};
```

The two fields most relevant to scheduling are:

- **`state`**: the current lifecycle state. Only processes in `RUNNABLE` state are eligible to run.
- **`context`**: the saved register state that gets restored when this process resumes after a context switch. More on this in Section 3.

Processes live in a fixed-size global array `struct proc proc[NPROC]` in `kernel/proc.c`. When you `fork()`, the kernel calls `allocproc()` to find an `UNUSED` slot and sets up the new process inside it.

---

## 2. Process Lifecycle States

```
           allocproc()
               │
               ▼
           ┌──────────┐
    ┌─────►│  UNUSED  │
    │      └──────────┘
    │           │
    │      fork()/allocproc()
    │           │
    │           ▼
    │      ┌───────────┐
    │      │  RUNNABLE │◄──────────────────┐
    │      └─────┬─────┘                   │
    │            │                         │
    │       scheduler()                    │
    │       (context switch)               │
    │            │                         │
    │            ▼                         │
    │      ┌───────────┐              timer interrupt
    │      │  RUNNING  │───────────────────► (yield)
    │      └─────┬─────┘                   │
    │            │                         │
    │       sleep()                   scheduler()
    │            │                         │
    │            ▼                         │
    │      ┌───────────┐                   │
    │      │ SLEEPING  │───── wakeup()────►│
    │      └───────────┘   (→ RUNNABLE)    │
    │                                       │
    │       exit()                          │
    │            │                          │
    │            ▼                          │
    │      ┌───────────┐                   │
    │      │  ZOMBIE   │                   │
    │      └─────┬─────┘                   │
    │            │                         │
    │       wait()—reaps child             │
    │            │                         │
    └────────────┘                         │
           (back to UNUSED)                │
                                           │
```

Transitions:
- **UNUSED → RUNNABLE**: `allocproc()` prepares a new process; `fork()` makes it visible.
- **RUNNABLE → RUNNING**: the scheduler picks this process and context-switches to it.
- **RUNNING → RUNNABLE**: a timer interrupt forces `yield()`, which saves the context and re-adds the process to the scheduler's pool.
- **RUNNING → SLEEPING**: the process calls `sleep()` to wait for an event (disk I/O, pipe read, etc.).
- **SLEEPING → RUNNABLE**: another process calls `wakeup()` on the matching channel.
- **RUNNING → ZOMBIE**: the process calls `exit()`; its resources are freed but its `struct proc` slot and exit status remain.
- **ZOMBIE → UNUSED**: the parent calls `wait()`, reads the exit status, and releases the slot.

---

## 3. The xv6 Scheduler

The scheduler lives in `kernel/proc.c` as the function `scheduler()`. It runs on **each CPU core** in an infinite loop. Here is the core logic, simplified:

```c
void
scheduler(void)
{
    struct proc *p;
    struct cpu *c = mycpu();

    for (;;) {
        // Avoid deadlock: re-enable interrupts so timer interrupts can happen
        intr_on();

        for (p = proc; p < &proc[NPROC]; p++) {
            acquire(&p->lock);
            if (p->state == RUNNABLE) {
                // Mark it as RUNNING on this CPU
                p->state = RUNNING;
                c->proc = p;

                // Context switch TO this process
                swtch(&c->context, &p->context);

                // We return here when this process is later scheduled OUT
                c->proc = 0;
            }
            release(&p->lock);
        }
    }
}
```

Key observations:

**Every process's `p->lock` is acquired before touching it.** The scheduler never looks at `p->state` without holding the lock. This is critical: another CPU might be modifying that same process's state (e.g., waking it up) at the same time.

**The `swtch` call does not return immediately.** It saves the scheduler's registers into `c->context` and loads the process's saved registers from `p->context`. The next time `swtch` is called _from the process side_ (inside `sched()`), it will save the process's current state and restore the scheduler's state, returning back to this loop — the line _after_ the `swtch` call.

**The loop iterates over all processes.** xv6 uses a simple round-robin policy: it starts from `proc[0]` each time and picks the first `RUNNABLE` process it finds. This means processes are scheduled in order of their array index by default.

---

## 4. Context Switching — `swtch.S`

The `swtch` function in `kernel/swtch.S` is written in assembly. It saves and restores **callee-saved registers** — the registers the RISC-V ABI says a function must preserve for its caller:

```asm
swtch:
    sd ra, 0(a0)       # return address
    sd sp, 8(a0)       # stack pointer
    sd s0, 16(a0)
    sd s1, 24(a0)
    sd s2, 32(a0)
    sd s3, 40(a0)
    sd s4, 48(a0)
    sd s5, 56(a0)
    sd s6, 64(a0)
    sd s7, 72(a0)
    sd s8, 80(a0)
    sd s9, 88(a0)
    sd s10, 96(a0)
    sd s11, 104(a0)

    ld ra, 0(a1)       # restore from new context
    ld sp, 8(a1)
    ld s0, 16(a1)
    ...                # (same pattern for s1–s11)
    ret
```

The first argument (`a0`) points to the **old** context (where to save). The second argument (`a1`) points to the **new** context (where to restore from). Since `ra` (the return address) is saved and restored, the `ret` at the end jumps to wherever the new context's `ra` points — which is usually a different location than where `swtch` was called from.

This means **a context switch is literally just saving and restoring 14 registers**. Everything else — page tables, file descriptors, the trapframe — stays in the `struct proc` and doesn't need to be swapped.

### Who calls `swtch`?

| Function | What it does | Direction |
|---|---|---|
| `scheduler()` | Calls `swtch(&c->context, &p->context)` | **Scheduler → Process** |
| `sched()` | Calls `swtch(&p->context, &c->context)` | **Process → Scheduler** |

`sched()` is called by `yield()`, `sleep()`, and `exit()` — whenever the currently running process wants to give up the CPU voluntarily or is forced to by a timer interrupt.

---

## 5. Spinlocks in xv6

Every kernel data structure in xv6 is protected by a **spinlock** (`struct spinlock` in `kernel/spinlock.h`). A spinlock is the simplest possible lock: if the lock is held, the caller spins in a tight loop ("busy-waits") until it's released.

```c
void acquire(struct spinlock *lk) {
    push_off();                  // disable interrupts on this CPU
    while (__sync_lock_test_and_set(&lk->locked, 1) != 0)
        ;                        // spin (busy-wait)
    __sync_synchronize();        // memory barrier
    lk->cpu = mycpu();          // record which CPU holds it
}

void release(struct spinlock *lk) {
    lk->cpu = 0;
    __sync_synchronize();        // memory barrier
    __sync_lock_release(&lk->locked);  // write 0 atomically
    pop_off();                   // re-enable interrupts
}
```

### Why disable interrupts?

Without `push_off()`, a deadlock scenario exists:

1. CPU A holds `lk`.
2. A timer interrupt fires, and the interrupt handler tries to acquire `lk`.
3. The handler spins forever — `lk` is held by the same CPU, which can't release it until the handler returns.

By disabling interrupts on the current CPU while holding any spinlock, xv6 prevents this. This is one of the trickiest and most important details in the kernel.

### Lock ordering

xv6 has an informal lock ordering convention: you must acquire `p->lock` before any other process-related locks, and you must never hold one process's lock while trying to acquire another (unless you're the scheduler, which does exactly that — carefully, one at a time).

Violating lock ordering causes **deadlock**: thread A holds lock 1 and waits for lock 2; thread B holds lock 2 and waits for lock 1. Neither makes progress.

---

## 6. Sleep and Wakeup

A process that cannot make progress — waiting for a pipe to have data, waiting for a child to exit, waiting for a disk read to complete — should not waste CPU cycles by spinning. Instead, it calls `sleep()`:

```c
void sleep(void *chan, struct spinlock *lk) {
    // lk must be held by the caller; it's usually p->lock

    // Record the sleep channel
    p->chan = chan;
    p->state = SLEEPING;

    // Release the caller's lock, then schedule
    sched();

    // We woke up. Re-acquire the lock before returning.
    p->chan = 0;
    // re-acquire lk...
}
```

The corresponding `wakeup()` function iterates over all processes and changes the state of any process sleeping on the matching channel from `SLEEPING` to `RUNNABLE`.

The tricky part: between `sleep()` setting `p->state = SLEEPING` and calling `sched()`, a wakeup could arrive on another CPU. If `p->state` were `SLEEPING` during that window, the wakeup would find it and set it to `RUNNABLE` — then `sched()` would immediately context-switch away, and when the process runs again it would find itself `RUNNABLE` (or already `RUNNING` again) instead of `SLEEPING`. This is handled by requiring the caller of `sleep()` to hold `p->lock`, and requiring the caller of `wakeup()` to acquire `p->lock` — the lock serializes the transition so no wakeup is lost.

---

## 7. The `yield()` Path

When a timer interrupt fires while a process is running in user space:

1. The RISC-V hardware delivers a timer interrupt → `usertrap()` in `kernel/trap.c`.
2. `usertrap()` sees `scause` is a timer interrupt (not a syscall) and calls `yield()`.
3. `yield()` acquires `p->lock`, sets the process state to `RUNNABLE`, and calls `sched()`.
4. `sched()` calls `swtch` to save the process's context and restore the scheduler's.
5. The `scheduler()` loop picks another `RUNNABLE` process to run.
6. Eventually, the scheduler loops back to this process and `swtch` restores its context.

This is the heart of **preemptive multitasking**: a process doesn't need to voluntarily give up the CPU — the timer interrupt takes it away. The process has no idea it was suspended; it just sees `getpid()` return a bit slower than expected.

---

## 8. References

- [xv6-riscv book](https://pdos.csail.mit.edu/6.828/2025/xv6/book-riscv-rev4.pdf) — Chapter 7 (Scheduling), Chapter 8 (Processes)
- [xv6-riscv source](https://github.com/mit-pdos/xv6-riscv) — `kernel/proc.c`, `kernel/proc.h`, `kernel/swtch.S`, `kernel/spinlock.c`, `kernel/spinlock.h`
- OSTEP, Chapters 7–10 (Scheduling Policies) and Chapters 26–29 (Concurrency, Locks)
- RISC-V Privileged Architecture spec — `mtimecmp`, timer interrupt handling, `sie`/`sip` register details
