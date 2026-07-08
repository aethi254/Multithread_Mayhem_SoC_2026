# Week 6 — Processes & Scheduling

> Goal of the week: understand how the xv6 kernel decides which process gets the CPU, trace the scheduler's decision-making path through real source code, and then — for the first time — change the policy that decides who runs.

Last week you added new system calls to xv6. You crossed the user/kernel boundary, added fields to `struct proc`, and read kernel state. This week you go deeper: you will read **the scheduler itself**, understand the context switch mechanism that makes multiprogramming work, and then change how xv6 chooses which process to run. You'll also meet xv6's spinlocks — the primitive that protects every kernel data structure — and learn why locking is inseparable from scheduling.

---

## What you'll know by the end of the week

- How the xv6 **scheduler** (`kernel/proc.c`) chooses the next process to run using round-robin
- What a **context switch** actually looks like at the register level (`kernel/swtch.S`)
- How the **process lifecycle** (RUNNABLE → RUNNING → SLEEPING → RUNNABLE) connects to scheduling
- How **spinlocks** work in xv6, and why the scheduler itself needs to acquire and release `p->lock`
- How the **sleep/wakeup** mechanism lets processes voluntarily give up the CPU
- How to modify the scheduler to implement a different policy

---

## What's in this folder

| File / folder | What it is |
|---|---|
| `theory.md` | All the concepts for the week. Read this first. |
| `problem-statement.md` | The four problems you'll work on. |
| `code/` | Example xv6 programs and a worked scheduler trace. |
| `sample-outputs/` | Reference output for each problem. |

---

## Suggested plan for the week

| Day | What to do |
|-----|------------|
| 1 | Read theory.md Sections 1–3. Open `kernel/proc.c` and trace the scheduler loop. |
| 2 | Read theory.md Sections 4–5 on context switching and spinlocks. Run `code/01_sched_trace.c` inside xv6. |
| 3 | Read theory.md Section 6. Work through `code/02_worked_example_sched_print.md` in your own checkout. |
| 4 | Start the problem set: Problem 1 (trace), then Problem 2 (priority scheduler). |
| 5 | Attempt Problem 3, and Problem 4 if you have time. Write your report. |

Read, then trace, then modify — same discipline as every previous week, with the stakes a little higher now that you're changing how the kernel hands out CPU time.

---

## How to compile and run the code examples

The example programs only run inside xv6:

```bash
cp week-6/code/01_sched_trace.c xv6-riscv/user/
# add $U/_01_sched_trace\ to UPROGS in the xv6 Makefile
cd xv6-riscv
make clean && make && make qemu
```

```
$ 01_sched_trace
```

The Makefile inside `week-6/code/` is provided for sanity-checking compilation with plain `gcc` — the binaries it produces cannot actually run on your host, since they depend on xv6-specific syscalls like `uptime()`.

---

## A word of warning before you begin

**You are modifying the scheduler this week.** A mistake here doesn't just break one program — it can make the entire system hang, crash, or fail to boot at all. Two habits will save you time:

1. **`make clean && make` after every kernel change.** Stale object files cause mystifying "my change isn't taking effect" bugs.
2. **One change at a time.** Get Problem 2 fully working before starting Problem 3. If something breaks, revert and rebuild until you isolate it.

If QEMU hangs on boot or you see `panic` messages about locks: that means your scheduler change introduced a concurrency bug. Read the panic, `Ctrl+A` then `X` to exit, review your locking discipline, and try again.

---

## Submission instructions

Submissions for this week will be collected via a **Google Form**. Your mentor will share the form link separately.

### Files to keep ready

| Problem | File(s) to keep ready |
|---|---|
| Problem 1 — Trace the scheduler | `sched_trace.md` |
| Problem 2 — Priority scheduler | Kernel diffs + `priority_test.c` |
| Problem 3 — Lock detective | Kernel diffs + `lock_detective.c` |
| Problem 4 — Schedviz *(if attempted)* | `schedviz.c` |

Also keep ready a short **report** — half a page is plenty — covering:

- Which problems you attempted
- Your observations from the scheduler trace in Problem 1
- How you implemented priority scheduling in Problem 2 and what you observed
- (If attempted) your findings from Problem 3 and 4
- One thing that surprised you this week
- Anything you got stuck on

### What's expected

- A clean `make clean && make` with no new warnings
- A working xv6 that boots and runs after your scheduler changes
- Test programs that demonstrate your changes work correctly
- Evidence that you read and understood the scheduler source — cite specific files and line numbers

### What's NOT expected

- A production-quality scheduler. A minimal, correct implementation is enough.
- Benchmarks with rigorous statistical analysis. Run your test 5–10 times and describe what you saw.
- All four problems. Problems 1 and 2 are the minimum.

### File hygiene

Submit your test programs and a `git diff` of your kernel changes — not the entire `xv6-riscv` tree. Do not submit compiled binaries.

---

## Stuck?

- **Kernel hangs on boot?** You probably broke the scheduler or introduced a lock ordering problem. Revert to a clean xv6 and re-apply your changes one at a time.
- **`panic: acquire` or `panic: release`?** A lock was acquired but not released, or two locks were acquired in inconsistent order. Check every code path through your scheduler changes.
- **Your priority test shows no difference?** Make sure you're running enough processes (use a fork bomb in a loop) and that your priority values span a wide enough range.
- **`make` succeeds but QEMU produces no output?** Try `make clean && make` — stale objects from an earlier build can silently corrupt behavior.
- Re-read `theory.md` if anything about `swtch`, `scheduler()`, or `sched()` is confusing — these are the load-bearing ideas this week.
- Still stuck? Open an issue or message your mentor.
