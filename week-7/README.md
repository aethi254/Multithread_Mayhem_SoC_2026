# Week 7 — Thread Design

> Goal of the week: design the data structures and mechanisms xv6 will need to support kernel-level threads — before writing `thread_create()` and `thread_join()` next week. Understand what a thread actually needs that a process doesn't already give you for free, and confirm that the context-switch machinery you studied in Week 6 needs no changes at all.

Weeks 4–6 gave you everything a *process* needs: an address space, system calls, a scheduler, locks. This week you stop and ask a narrower question: **what exactly is different about a thread?** The answer turns out to be small — a thread is a process that shares its address space with others instead of getting a private copy — but that small difference has real consequences for stacks, process bookkeeping, and memory safety. Week 8 is where you wire up `thread_create`/`thread_join` end-to-end; this week is where you make sure you understand every piece you're about to connect.

---

## What you'll know by the end of the week

- The precise difference between what `fork()` gives a **process** (a private copy of the address space, made by `uvmcopy()`) and what a **thread** needs instead (a *shared* address space)
- Why xv6's existing context-switch mechanism (`swtch`, `scheduler()`, `sched()` — all from Week 6) needs **zero changes** to support threads, and exactly which two things *do* need to change
- How to extend `struct proc` with thread-group bookkeeping (`tgid`, `is_thread`) so the kernel can tell which `struct proc` slots belong to the same "process" in the traditional sense
- How to allocate a fresh, independent **user stack** inside an already-running process's address space, using the same `uvmalloc()`/`uvmunmap()` primitives that back `sbrk()`
- Why an unguarded stack allocation is dangerous, how a **guard page** turns silent memory corruption into an immediate, catchable page fault, and what that fault looks like from inside xv6

---

## What's in this folder

| File / folder | What it is |
|---|---|
| `theory.md` | All the concepts for the week. Read this first, especially Sections 3 and 5. |
| `problem-statement.md` | The four problems you'll work on. |
| `code/` | A stock-xv6 address-space probe, plus a fully worked walkthrough of adding a trivial thread-bookkeeping syscall. |
| `sample-outputs/` | Reference output for the code example and for one possible implementation of each problem. |

---

## Suggested plan for the week

| Day | What to do |
|-----|------------|
| 1 | Read `theory.md` Sections 1–2. Run `code/01_addrspace_probe.c` inside xv6 and watch `sbrk()` move the top of the address space. |
| 2 | Read `theory.md` Sections 3–4 closely — this is where "process" and "thread" actually diverge. |
| 3 | Read `theory.md` Section 5. Work through `code/00_worked_example_am_i_a_thread.md` in your own checkout, build it, run it. |
| 4 | Start the problem set: Problem 1 (design/trace), then Problem 2 (thread-group fields + syscall). |
| 5 | Attempt Problem 3, and Problem 4 if you have time. Write your report. |

Same discipline as every previous week: read, then run the worked example, then trace, then build your own. This week's kernel changes are small, but they're the foundation Week 8 builds `thread_create`/`thread_join` directly on top of — get them right.

---

## How to compile and run the code examples

`01_addrspace_probe.c` runs on **stock xv6** — no kernel changes needed for it.

```bash
cp week-7/code/01_addrspace_probe.c xv6-riscv/user/
# add $U/_01_addrspace_probe\ to UPROGS in the xv6 Makefile
cd xv6-riscv
make clean && make && make qemu
```

```
$ 01_addrspace_probe
```

The Makefile inside `week-7/code/` is provided for reference only, in case you want to sanity-check that a file compiles with plain `gcc` before copying it into the xv6 tree — it cannot actually run the resulting binary on your host, since `sbrk()` behaves differently outside xv6 and the problem-set syscalls (`alloc_thread_stack`, `getthreadinfo`, ...) don't exist there at all.

---

## A word of warning before you begin

**You are extending `struct proc` and touching the address-space allocator this week.** Getting a field's initialization wrong, or getting a page-rounding calculation wrong in a stack allocator, tends to fail quietly — the kernel boots fine and only misbehaves under a specific test, which is worse than an outright panic. Two habits will save you real time:

1. **`make clean && make` after every kernel-side change.** You're editing `kernel/proc.h`, which is included almost everywhere — stale objects here cause the most confusing bugs of any week so far.
2. **Print everything while you build.** Before trusting `alloc_thread_stack()`'s return value, `printf` the requested size, the rounded size, and `p->sz` before and after, from inside the kernel. Silent off-by-one-page bugs are the norm, not the exception, in memory-layout code.

If QEMU panics or a test process dies unexpectedly this week, that's often the *point* — Problem 4 deliberately triggers a page fault to prove a guard page works. Read the panic/fault message (it names the file, line, or `scause` value), `Ctrl+A` then `X` to exit, and check it matches what the theory pack predicts before assuming something is broken.

---

## Submission instructions

Submissions for this week will be collected via a **Google Form**. Your mentor will share the form link separately.

### Files to keep ready

| Problem | File(s) to keep ready |
|---|---|
| Problem 1 — Address-space design trace | `thread_design.md` |
| Problem 2 — Thread-group bookkeeping | Kernel diffs + `threadinfo_test.c` |
| Problem 3 — Thread stack allocator | Kernel diffs + `stack_alloc_test.c` |
| Problem 4 — Guard pages *(if attempted)* | Kernel diffs + `guardpage_test.c` |

Also keep ready a short **report** — half a page is plenty — covering:

- Which problems you attempted
- Your design answer from Problem 1 — what would have to change to make `fork()` share instead of copy
- What you observed from `stack_alloc_test` — did the two stacks come out exactly adjacent, and why
- (If attempted) what the guard-page fault actually printed in Problem 4, and whether it matched your prediction
- One thing that surprised you
- Anything you got stuck on

### What's expected

- A clean `make clean && make` with no new warnings
- A working xv6 that boots after your `struct proc` and allocator changes
- Evidence you read `uvmcopy()`, `uvmalloc()`, and `uvmunmap()` in the actual source before writing your own allocator — cite specific files and line numbers in Problem 1
- Test programs whose output you can explain, not just reproduce

### What's NOT expected

- A working `thread_create()` or `thread_join()` — that's all of Week 8
- Scheduler changes — the scheduler you built in Week 6 is untouched this week
- A perfectly general-purpose allocator. A stack allocator that only ever grows and never frees is fine
- All four problems. Problems 1 and 2 are the minimum

### File hygiene

Submit your test programs and a `git diff` of your kernel changes — not the entire `xv6-riscv` tree. Do not submit compiled binaries.

---

## Stuck?

- **`make` fails with errors in `proc.h`?** A missing semicolon or a field declared inside the wrong struct is the usual cause — `struct proc` and `struct cpu` are easy to mix up when skimming.
- **`getthreadinfo` always returns garbage?** Check you're using `copyout` with the *address* of your local struct and `sizeof(...)` of the struct, not of a pointer to it.
- **Your two stacks from Problem 3 overlap instead of sitting side by side?** You likely returned the wrong end of the mapped range — remember stacks grow **down**, so the syscall should return the *top* (highest address), and the next call's `oldsz` needs to already reflect the previous allocation.
- **Problem 4's guard page never faults?** Double-check `uvmunmap` was called on the *lowest* page of the new region, not the highest, and that you're writing to an address *below* the returned top by more than `size` bytes.
- **`panic: uvmunmap: not mapped`?** You're trying to unmap a page that was never mapped, usually because of an off-by-one in your page-rounding math. Print the exact address you're passing before calling `uvmunmap` and compare it to what `uvmalloc` actually mapped.
- Re-read `theory.md` Section 5 if the "why does the scheduler need no changes" idea feels shaky — that's the load-bearing idea this week, and Week 8 assumes you're comfortable with it.
- Still stuck? Open an issue or message your mentor.
