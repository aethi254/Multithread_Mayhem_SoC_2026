# Week 5 — System Calls

> Goal of the week: trace exactly what happens, step by step, when a user
> program crosses into the kernel — and then make that boundary do
> something new by adding your own system call.

Week 4 was about looking at xv6 from the outside: booting it, reading its
source tree, writing ordinary user programs. This week you go one layer
deeper. You will trace the real trap path through real source files, and
then — for the first time in this project — **modify the kernel itself**.

---

## What you'll know by the end of the week

- The exact sequence of steps from a user program's `getpid()` call to the
  kernel's `sys_getpid()` and back — registers, trapframes, and all
- Why the kernel cannot safely use a user-space pointer directly, and what
  `copyin`/`copyout` do about it
- How the syscall dispatch table (`syscalls[]` in `kernel/syscall.c`) maps
  numbers to functions
- How to add a brand-new system call to xv6, end to end, across all five
  files that need to agree with each other
- How to safely extract integer, pointer, and string arguments from a
  trapframe using `argint`, `argaddr`, and `argstr`

---

## What's in this folder

| File / folder | What it is |
|---|---|
| `theory.md` | All the concepts for the week. Read this first, especially Sections 4 and 5. |
| `problem-statement.md` | The four problems you'll work on. |
| `code/` | Two example xv6 user programs, plus a fully worked walkthrough of adding a trivial syscall. |
| `sample-outputs/` | Reference output for the code examples and for one possible implementation of each problem. |

---

## Suggested plan for the week

| Day | What to do |
|-----|------------|
| 1 | Read theory.md Sections 1–2. Run `01_arg_passing_demo` and `02_self_timing_demo` inside xv6. |
| 2 | Read theory.md Sections 3–4 closely — these are the hardest ideas this week. |
| 3 | Read theory.md Section 5. Work through `code/00_worked_example_hello_syscall.md` in your own checkout, build it, call it, see both printed lines. |
| 4 | Start the problem set: Problem 1 (trace), then Problem 2 (your first real syscall). |
| 5 | Attempt Problem 3, and Problem 4 if you have time. Write your report. |

Read, then run the worked example, then trace, then build your own — same
"read before you touch the kernel" discipline as every previous week, just
with higher stakes now that you're editing kernel source.

---

## How to compile and run the code examples

`01_arg_passing_demo.c` and `02_self_timing_demo.c` only run inside xv6 —
they use xv6-specific syscalls (`uptime`) that don't exist on your host OS.

```bash
cp week-5/code/01_arg_passing_demo.c xv6-riscv/user/
cp week-5/code/02_self_timing_demo.c xv6-riscv/user/
# add $U/_01_arg_passing_demo\ and $U/_02_self_timing_demo\ to UPROGS in the xv6 Makefile
cd xv6-riscv
make clean && make && make qemu
```

```
$ 01_arg_passing_demo
$ 02_self_timing_demo
```

The Makefile inside `week-5/code/` is provided for reference only, in case
you want to sanity-check that a file compiles with plain `gcc` before
copying it into the xv6 tree — it cannot actually run the resulting binary
on your host, since `uptime()` and friends only exist inside xv6.

---

## A word of warning before you begin

**You are editing the kernel this week.** A mistake here doesn't just make
one program misbehave — it can panic the whole OS, or worse, build
"successfully" while quietly corrupting kernel state. Two habits will save
you real time:

1. **`make clean && make` after every kernel-side change.** Stale object
   files from a previous build are a frequent source of "I fixed it but
   it's still broken" confusion.
2. **Add one syscall at a time.** Get the worked `hello()` example fully
   working before starting Problem 2. Get Problem 2 fully working before
   starting Problem 3. If something breaks, you'll know exactly which
   change caused it.

If QEMU panics: that's normal this week, not a sign you've broken
something unrecoverable. Read the panic message (it names a file and
line), `Ctrl+A` then `X` to exit, fix your code, rebuild, try again.

---

## Submission instructions

Submissions for this week will be collected via a **Google Form**. Your
mentor will share the form link separately.

### Files to keep ready

| Problem | File(s) to keep ready |
|---|---|
| Problem 1 — Trace a syscall by hand | `syscall_trace.md` |
| Problem 2 — Syscall counter | Your kernel diffs + `getsyscount_test.c` |
| Problem 3 — Process peeker | Your kernel diffs + `procpeek_test.c` |
| Problem 4 — Safe copy-out *(if attempted)* | Your kernel diffs + `getcmdline_test.c` |

Also keep ready a short **report** — half a page is plenty — covering:

- Which problems you attempted
- Your answer to the "does it count itself" question in Problem 2
- What process state you observed in Problem 3, and whether it matched
  your expectation
- (If attempted) your explanation for Problem 4
- One thing that surprised you
- Anything you got stuck on

### What's expected

- A clean `make clean && make` with no new warnings
- Syscalls that work when called from a test program inside QEMU
- Evidence you used `argint`/`argaddr`/`copyout` correctly — no raw
  pointer dereferences on user memory anywhere in your kernel diffs

### What's NOT expected

- Scheduler or memory-management changes — those come in later weeks
- A production-quality implementation. Minimal and correct is the bar.
- All four problems. Problems 1 and 2 are the minimum.

### File hygiene

Submit your test programs and only the specific kernel files you changed
(or a `git diff`) — not the entire `xv6-riscv` tree. Do not submit compiled
binaries.

---

## Stuck?

- Re-read `theory.md` Section 4 if anything involving `copyin`/`copyout`
  or user pointers feels shaky — that's the load-bearing idea this week.
- Compare your output to `sample-outputs/` for the example or problem
  you're working on.
- `make` succeeds but linking your user program fails on an unresolved
  syscall name? You probably forgot the `usys.pl` entry.
- Check the GitHub Issues — someone may have hit the same thing.
- Still stuck? Open an issue or message your mentor. Kernel bugs are
  genuinely harder to read than user-space bugs — asking for help here is
  expected, not a failure.
