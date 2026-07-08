# Week 6: Problem Set — Processes & Scheduling

This week you go inside the xv6 scheduler. You'll trace its path through the source, then modify it to implement a new scheduling policy. Every problem below requires editing files inside `kernel/`, not just `user/`.

**Read `theory.md` in full before starting, especially Sections 3–6.** Problems 3 and 4 will not make sense without them.

---

## Overview

| # | Problem | Focus | Required? |
|---|---|---|---|
| 01 | Trace the scheduler | Reading the scheduler loop and context switch path in real code | Yes |
| 02 | Priority scheduler | Modify the scheduler to support process priority | Yes |
| 03 | Lock detective | Add a syscall to inspect whether a spinlock is held | Recommended |
| 04 | Schedviz | Visualize scheduling decisions from user space | Stretch |

Problems 1 and 2 are the minimum. Problem 3 is strongly encouraged. Problem 4 is optional but rewarding.

---

## Setup

You're reusing the same xv6 checkout from previous weeks. No new toolchain steps.

```bash
cd xv6-riscv
make clean && make && make qemu
```

Before touching any kernel file, read the worked example at `code/01_sched_trace.c` — it's a user-space program that reads scheduling-related kernel state.

---

## Problem 01 — Trace the scheduler

**WARM-UP · REQUIRED**

This problem requires no new code — it's a source-reading exercise that traces the full scheduler round trip. Pick **one process** (any PID you like) and trace its path through the scheduler lifecycle as if you were following it at the instruction level.

### What to do

Open the xv6 source files and trace the following, citing the file and approximate line number for each checkpoint:

1. **Process creation.** The process starts when `fork()` calls `allocproc()`. Find `allocproc()` in `kernel/proc.c`. How does it find a free `proc[]` slot? What fields does it initialize? What state is the process in after `allocproc()` returns?

2. **Becoming RUNNABLE.** The new process is added to the scheduler's pool. Where does `fork()` set `p->state = RUNNABLE`? Why must it hold `p->lock` during this transition?

3. **The scheduler picks it up.** Find the `scheduler()` function in `kernel/proc.c`. Describe its loop. What condition does it check before context-switching to a process? What does it do with `p->lock` before and after the context switch?

4. **The context switch to the process.** Open `kernel/swtch.S`. What registers does `swtch` save? What registers does it restore? How does `swtch` know where to start executing in the new process?

5. **Running.** The process now runs in user space. While it runs, where is the scheduler? (Specifically: what is the `c->context` for this CPU doing while the process executes?)

6. **Yielding the CPU.** A timer interrupt fires. Trace from `usertrap()` in `kernel/trap.c` through `yield()` to `sched()`. What state does the process transition to? What does `sched()` call, and what happens when `swtch` returns in the scheduler?

7. **The scheduler picks another process.** Back in `scheduler()`, the code continues right after the `swtch` call. What cleanup does it do? How does the loop find the next `RUNNABLE` process?

### What to submit

A markdown or plain-text file, `sched_trace.md`, with each of the 7 checkpoints above as its own short section: file name, approximate line number, and 2–3 sentences describing what happens there in your own words. Don't copy the theory pack — cite the actual lines you found in your own checkout.

---

## Problem 02 — Priority scheduler

**REQUIRED**

Modify xv6's scheduler to support **priority-based scheduling**. Each process gets a priority value (an integer, say 0–63), and the scheduler always picks the `RUNNABLE` process with the **highest priority** (lowest numeric value) instead of scanning from `proc[0]` and taking the first match.

### What to do

1. Add a `int priority;` field to `struct proc` in `kernel/proc.h`.
2. In `allocproc()` in `kernel/proc.c`, initialize the priority to a default value (e.g., 31 — mid-range).
3. Add a system call `set_priority(int pid, int prio)` that changes the priority of a given process. Return the old priority on success, or `-1` if no process with that `pid` exists.
4. Modify the `scheduler()` loop so it scans all processes and picks the `RUNNABLE` one with the smallest `priority` value (i.e., highest priority). If multiple processes have the same priority, pick the one with the lowest PID (or the first encountered — document your choice).
5. Add a system call `get_priority(int pid)` that returns the priority of a given process.
6. Wire up both syscalls through the usual five touch points (`syscall.h`, `user.h`, `usys.pl`, `syscall.c`, `sysproc.c`).

### A question to answer in your report

What happens if a process calls `set_priority()` on itself? Does the change take effect immediately, or only after the next timer interrupt forces a reschedule? Think about where you placed your priority check in the scheduler and whether a running process can preempt itself.

### Things to watch out for

- **Locking**: your scheduler loop must hold `p->lock` while reading `p->state` and `p->priority`, just like the original holds it while reading `p->state`. Don't acquire locks for every process at once — acquire and release one at a time as the original does.
- **Fairness**: processes with the same priority should still get roughly equal CPU time under round-robin. Document whether your implementation preserves this.

### Test it

Write `priority_test.c` that:

- Forks two child processes.
- Sets one child's priority to a high value (e.g., `5`) and the other to a low value (e.g., `50`).
- Each child does CPU-bound work (a long loop calling `getpid()` or doing arithmetic) and tracks how many iterations it completes.
- Prints how many iterations each child completed. The higher-priority child should complete noticeably more iterations.

```
$ priority_test
Parent: set child 4 to priority 5, child 5 to priority 50
Child 4 (prio 5): completed 84523 iterations
Child 5 (prio 50): completed 3201 iterations
Child 4 ran about 26x more than child 5
```

### What to submit

Your kernel diffs for `kernel/proc.h`, `kernel/proc.c`, `kernel/syscall.h`, `kernel/syscall.c`, `kernel/sysproc.c`, `user/user.h`, and `user/usys.pl`, plus `priority_test.c` and your answer to the self-priority question in your report.

---

## Problem 03 — Lock detective

**RECOMMENDED**

Add a system call `is_lock_held(int lock_id)` that tells a user program whether a given kernel spinlock is currently held. For this problem, we'll focus on three well-known locks: the **ticks lock** (`tickslock`), the **process table lock** (the lock protecting `proc[]` — actually each process has its own lock, so we'll use `initlock`s naming convention), and the **console lock** (`cons.lock`).

Since user space cannot access kernel memory directly, this syscall reveals indirect information about kernel state — a capability that real systems expose through `/proc/lock_stat` or `LOCKDEP` output.

### What to do

1. Add a system call `is_lock_held(int lock_id)` that takes an integer identifier:
   - `0` → check `tickslock`
   - `1` → check the console's `cons.lock`
   - Any other value → return `-1` (invalid lock id)
2. In the kernel side, use a helper that returns `lk->locked` (the raw field of `struct spinlock`). Since `p->lock` is per-process and you'd need to say which process, we'll stick with the two global locks.
3. Wire through the usual five touch points.

**You'll need to `extern` declare `tickslock` and `cons.lock` in your sysproc.c file** — they're defined in `kernel/timer.c` and `kernel/console.c` respectively.

### A question to answer in your report

Can `is_lock_held()` give a **false negative** — return `0` even though the lock is logically "held" by another CPU? (Hint: think about atomicity and the fact that you're reading `lk->locked` without holding any lock of your own.) Explain your reasoning.

### Test it

Write `lock_detective.c` that:

- Calls `is_lock_held(0)` (tickslock) and prints the result.
- Calls `is_lock_held(1)` (cons.lock) and prints the result.
- Calls `is_lock_held(99)` (invalid) — should return `-1`.
- Then sleeps for a few ticks, calls `is_lock_held(0)` again, and checks if the result ever differs (it may not — that's worth noting in your report).

```
$ lock_detective
tickslock (id 0): 0 (not held)
cons.lock (id 1): 0 (not held)
invalid (id 99):  -1
After sleeping: tickslock (id 0): 0 (not held)
```

### What to submit

Your kernel diffs, `lock_detective.c`, and a paragraph in your report explaining whether `is_lock_held()` can give a false negative and why.

---

## Problem 04 — Schedviz

**STRETCH**

Write a user-space program `schedviz.c` that **visualizes scheduling decisions** by repeatedly calling `uptime()` and tracking how much CPU time each of several child processes gets.

### What to do

1. Fork N child processes (N = 4 is a good default). Each child runs a tight loop that just calls `getpid()` in a counter loop until a shared flag tells it to stop.
2. Each child tracks how many loop iterations it completed.
3. After a fixed duration (use `sleep(50)` in the parent — about 5 seconds at xv6's default tick rate), set the stop flag so children finish.
4. Each child prints its PID and iteration count.
5. The parent prints a summary bar chart: for each child, print a row of `#` characters proportional to its iteration count.

**Interesting twist:** Use `set_priority()` from Problem 2 to change child priorities between runs and compare the bar charts. Does a higher-priority child get proportionally more CPU?

### Expected output

```
$ schedviz 4
Running 4 children for 50 ticks...
Child 3: 14237 iterations  ████████████████████████████████
Child 4: 13891 iterations  ██████████████████████████████
Child 5: 14012 iterations  ███████████████████████████████
Child 6: 13988 iterations  ██████████████████████████████
```

With priority manipulation:

```
$ schedviz 4 5
(Child 3 gets priority 5, others stay at default 31)
Child 3: 84732 iterations  ████████████████████████████████████████████████████████
Child 4: 3210 iterations   ██
Child 5: 3301 iterations   ██
Child 6: 3198 iterations   ██
```

### Hints

- Use shared memory (a `volatile int` in a page allocated by `sbrk()` that's shared after `fork()`) for the stop flag. xv6 doesn't have `mmap`, but `sbrk` works.
- The bar chart can be approximate — you don't need precise scaling, just a rough visual.
- `uptime()` returns ticks since boot. Use it before and after the measurement window to report how many ticks actually elapsed.

### What to submit

`schedviz.c` and any notes about what you observed when running with and without priority changes.

---

## Submission

Keep ready for the Google Form your mentor will share:

| Problem | File(s) to keep ready |
|---|---|
| Problem 1 — Trace the scheduler | `sched_trace.md` |
| Problem 2 — Priority scheduler | Kernel diffs + `priority_test.c` |
| Problem 3 — Lock detective | Kernel diffs + `lock_detective.c` |
| Problem 4 — Schedviz *(if attempted)* | `schedviz.c` |

"Kernel diffs" means: either `git diff` output from your xv6 checkout, or the full modified files (`proc.h`, `proc.c`, `syscall.h`, `syscall.c`, `sysproc.c`, `usys.pl`, `user.h`) — whichever your mentor's form asks for. Don't submit the entire xv6 tree.

Also keep ready a short **report** covering:

- Which problems you attempted
- Your observations from the scheduler trace in Problem 1
- How you implemented priority scheduling and what you observed from `priority_test`
- Your answer to the self-priority question in Problem 2
- (If attempted) your findings from Problems 3 and 4
- One thing that surprised you
- Anything you got stuck on

### What's expected

- A clean `make clean && make` with no new warnings
- A working xv6 that boots after your scheduler modifications
- Evidence that you tested your priority scheduler with a multi-process workload
- Evidence that you read the actual scheduler source (cite file names and line numbers in Problem 1)

### What's NOT expected

- Complex scheduling policies. A simple priority-based scan is enough.
- Fairness or starvation guarantees. If a low-priority process never runs, document it and explain why.
- All four problems. Problems 1 and 2 are the minimum.

### File hygiene

Submit your test programs and the specific files you changed — not the entire `xv6-riscv` tree.

---

## Stuck?

- **Kernel hangs on boot after your change?** The most likely cause is a broken scheduler loop. Revert to stock xv6 and re-apply your changes one function at a time.
- **`panic: acquire` on boot?** You probably forgot to initialize a spinlock, or your scheduler loop holds a lock across `swtch` (it shouldn't — the original doesn't).
- **Priority test shows no difference?** Make your priority gap wider (e.g., 0 vs 63) and increase the runtime. xv6's timer tick is coarse (every 10ms by default).
- **`set_priority` returns `-1`?** Check that you're searching the full `proc[]` array and correctly handling the case where `pid` matches but the slot is `UNUSED`.
- **Can't find a lock by name?** Use `grep` in the xv6 source: `grep -n "struct spinlock" kernel/*.c` to find where locks are declared.
- Still stuck? Message your mentor.
