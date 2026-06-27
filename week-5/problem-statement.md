# Week 5: Problem Set — System Calls

This is the first week you modify the **kernel**, not just user space. Every
problem below requires editing files inside `kernel/`, not just `user/`.
Take a clean copy of your xv6 checkout, or commit/stash your Week 4 work
first — kernel panics this week can be confusing to debug if you're not sure
which change caused them.

**Read `theory.md` in full before starting, especially Sections 4 and 5.**
Problems 3 and 4 will not make sense without them.

---

## Overview

| # | Problem | Focus | Required? |
|---|---|---|---|
| 01 | Trace a syscall by hand | Reading the trap path in real code | Yes |
| 02 | Syscall counter | Adding your first syscall, no arguments | Yes |
| 03 | Process peeker | Syscall with an `int` argument + kernel-wide state | Recommended |
| 04 | Safe copy-out | Syscall with a pointer argument + `copyout` | Stretch |

Problems 1 and 2 are the minimum. Problem 3 is strongly encouraged — it's
the most representative "real" kernel work of the week. Problem 4 is
optional but is the one that most directly tests whether you understood
Section 4 of the theory.

---

## Setup

You're reusing the same xv6 checkout from Week 4. No new toolchain steps —
if `make qemu` from last week still boots, you're ready.

```bash
cd xv6-riscv
make clean && make && make qemu
```

Before touching any kernel file, read the worked example at
`code/00_worked_example_hello_syscall.md`. It adds a syscall called
`hello()` end-to-end. Follow along in your own checkout, build it, and call
it from a throwaway test program before starting Problem 2. The problems
below assume you've done this once already and won't re-explain the five
touch points.

---

## Problem 01 — Trace a syscall by hand

**WARM-UP · REQUIRED**

This problem requires no new code. Pick **one** of `sys_getpid` or
`sys_uptime` (your choice — both are short and self-contained) and trace its
entire round trip through real source, citing the file and approximate
line number for each checkpoint below.

### What to do

Starting from a user program calling, say, `getpid()`, trace and write down:

1. Where the user-space stub lives, and what two things it does before
   trapping (`user/usys.S` — note this file is generated; also look at
   `user/usys.pl` to see where the entry comes from).
2. The `ecall` instruction itself — what changes in hardware state
   (privilege, `sepc`, `scause`, `stvec`) the moment it executes. You won't
   find this as a single line of xv6 source; describe it from the RISC-V
   behavior, citing the theory pack or the RISC-V spec.
3. The first xv6 instruction that runs afterward — `uservec` in
   `kernel/trampoline.S`. What does it save, and where?
4. The C function that decides this is a syscall and not some other trap —
   `usertrap()` in `kernel/trap.c`. Quote the condition it checks.
5. Where the syscall number is read and the function pointer is found —
   `syscall()` in `kernel/syscall.c`.
6. The actual implementation — `sys_getpid()` or `sys_uptime()` in
   `kernel/sysproc.c`. How many lines is it? What does it return, and
   where does that return value end up (which register, which struct
   field)?
7. The return path — name the two functions involved in getting back to
   user mode (`usertrapret()` and the trampoline's `userret`), and what
   instruction finally drops back to U-mode.

### What to submit

A markdown or plain-text file, `syscall_trace.md`, with each of the 7
checkpoints above as its own short section: file name, approximate line
number, and 1–3 sentences describing what happens there in your own words.
Don't just copy the theory pack back at us — cite the actual lines you
found in your own checkout.

---

## Problem 02 — Syscall counter

**REQUIRED**

Add a new system call, `getsyscount()`, that takes no arguments and returns
the total number of system calls the **calling process** has made since it
started (including, awkwardly, the call to `getsyscount()` itself — more on
that below).

### What to do

1. Add a new field to `struct proc` in `kernel/proc.h`, e.g.
   `int syscalls;` — a per-process counter.
2. Initialize it to `0` wherever the rest of a process's fields are reset
   when a slot is allocated — look at `allocproc()` in `kernel/proc.c`.
3. Increment it inside `syscall()` in `kernel/syscall.c`, once per call,
   regardless of which syscall it was or whether the number was valid.
4. Add the five standard touch points (Section 5 of the theory pack) for a
   new syscall named `getsyscount`, with no arguments, returning
   `p->syscalls`.

### A question to answer in your report

Because the increment lives inside `syscall()` itself, the call to
`getsyscount()` increments the very counter it's about to return. Decide
what you think the *correct* behavior is — should the call to
`getsyscount()` count itself or not? — and either:

- leave it as is and explain in your report why you think that's
  reasonable, or
- adjust your implementation so it doesn't count itself, and explain how
  you did it (hint: where would you need to read the counter relative to
  where you increment it?).

There's no single "right" answer expected here — we're looking for the
reasoning, not a specific number.

### Test it

Write `getsyscount_test.c`: call a handful of syscalls you already know
(`getpid()`, `uptime()`, `sleep(0)`), then call `getsyscount()` and print
the result. Run it more than once and confirm the count increases by
exactly the number of calls you added when you add more.

### What to submit

The diffs/new code for `kernel/proc.h`, `kernel/proc.c`, `kernel/syscall.h`,
`kernel/syscall.c`, `kernel/sysproc.c`, `user/user.h`, and your
`user/usys.pl` entry, plus `getsyscount_test.c` and your answer to the
question above in your report. (You don't need to submit the entire xv6
tree — see the submission section in `README.md` for exactly which files
to package.)

---

## Problem 03 — Process peeker

**RECOMMENDED**

Add a system call, `getprocstate(int pid)`, that returns the current
`procstate` (as an `int`) of the process with the given `pid`, or `-1` if no
process with that `pid` currently exists.

### What to do

This syscall takes a real argument this time — your first use of `argint`.
It also needs to look beyond the calling process: it has to search the
**entire process table**, not just `myproc()`.

We recommend you do this the way `kill()` already does it (read `kill()` in
`kernel/proc.c` before writing any code — it solves almost the same
problem: "find the proc with this pid, touch it safely, handle the
not-found case"):

1. Write a helper function in `kernel/proc.c`, something like
   `int getprocstatebypid(int pid)`, that loops over the global `proc[]`
   array, `acquire`s each process's lock before reading its `pid` and
   `state`, `release`s it again, and returns the state of the first match
   (or `-1` if the loop finishes with no match).
2. Declare that helper in `kernel/proc.h` so `sysproc.c` can call it.
3. Implement `sys_getprocstate()` in `kernel/sysproc.c`: use `argint(0, &pid)`
   to fetch the argument, call your helper, return the result.
4. Wire through the usual five touch points for the syscall itself.

### Why the locking matters

Other CPUs (or other code on this CPU, if you get preempted) can be
modifying any process's state at any moment — a process you're looking at
could be exiting right as you read it. `acquire`/`release` around each
individual `p->lock` is what makes the read safe. Don't hold a lock for
longer than you need to, and don't try to hold more than one process's lock
at a time in this loop — that's how deadlocks are made.

### Test it

Write `procpeek_test.c`: `fork()` a child, have the **parent** sleep
briefly (so the child has time to actually start running), then call
`getprocstate(childpid)` from the parent and print the result. Also call
`getprocstate()` with a `pid` you're confident doesn't exist (e.g. `9999`)
and confirm you get `-1`. `wait()` for the child before exiting.

The numeric values of `enum procstate` (`kernel/proc.h`) aren't
self-explanatory on their own — print both the number and, in your test
program, a small lookup table mapping it back to a name (`UNUSED`,
`SLEEPING`, `RUNNABLE`, `RUNNING`, `ZOMBIE`) so the output is readable.

### What to submit

Your kernel diffs, `procpeek_test.c`, and a short note in your report
describing what state you observed the child in and whether it matched
what you expected.

---

## Problem 04 — Safe copy-out

**STRETCH**

Add a system call, `getcmdline(char *buf, int max)`, that copies the
calling process's name (the `p->name` field — the same name you'd see in
`ps`-style output, set from the program's `argv[0]` at `exec` time) into a
user-supplied buffer, NUL-terminated and truncated to fit in `max` bytes if
necessary. Return the number of bytes copied (not counting the NUL), or
`-1` if `max` is invalid (`<= 0`) or the copy itself fails.

### Why this one is the stretch problem

This is the syscall that actually needs `copyout`. `p->name` lives in
**kernel memory** (it's a field inside `struct proc`). `buf` is a pointer
the *user* program owns, in *user* memory, in a different page table. You
must not write `p->name` into `*buf` directly — re-read Section 4 of the
theory pack on exactly why, then use `copyout()`.

### What to do

1. Add the five touch points for `getcmdline(char *buf, int max)`.
2. In `sys_getcmdline()`: use `argaddr(0, &buf_va)` to get the user
   pointer (as a `uint64`, not a real pointer — it's not valid in the
   kernel's address space) and `argint(1, &max)` for the size.
3. Validate `max > 0` and return `-1` early if not.
4. Compute how many bytes you'll actually copy: the shorter of
   `strlen(p->name)` and `max - 1` (leaving room for the NUL).
5. Call `copyout(p->pagetable, buf_va, p->name, n)` to copy the name bytes,
   then a second `copyout` call to write a single NUL byte at
   `buf_va + n`. Check the return value of both calls — if either is
   negative, return `-1`.
6. Return `n` on success.

### Test it

Write `getcmdline_test.c` that:

- Calls `getcmdline(buf, 64)` with a generously-sized buffer and prints the
  result — it should print back its own program name.
- Calls it again with a tiny buffer (`getcmdline(buf, 4)`) and prints the
  (truncated) result, to demonstrate the truncation path actually works.

### What to submit

Your kernel diffs, `getcmdline_test.c`, and a paragraph in your report
explaining, in your own words, why writing directly through `buf` inside
the kernel would have been wrong — and what you think would happen if you
tried it anyway (you don't have to actually try it and crash your kernel,
though you're welcome to, carefully, on a throwaway build).

---

## Submission

Keep ready for the Google Form your mentor will share:

| Problem | File(s) to keep ready |
|---|---|
| Problem 1 — Trace a syscall by hand | `syscall_trace.md` |
| Problem 2 — Syscall counter | Kernel diffs + `getsyscount_test.c` |
| Problem 3 — Process peeker | Kernel diffs + `procpeek_test.c` |
| Problem 4 — Safe copy-out | Kernel diffs + `getcmdline_test.c` |

"Kernel diffs" means: either `git diff` output from your xv6 checkout, or
the full modified files (`proc.h`, `proc.c`, `syscall.h`, `syscall.c`,
`sysproc.c`, `usys.pl`, `user.h`) — whichever your mentor's form asks for.
Don't submit the entire xv6 tree.

Also keep ready a short **report** covering:

- Which problems you attempted
- Your answer to the "does it count itself" question in Problem 2
- What state you observed in Problem 3 and whether it matched your
  expectation
- (If attempted) your explanation for Problem 4
- One thing that surprised you
- Anything you got stuck on

### What's expected

- A clean `make clean && make` with no new warnings
- Syscalls that work when called from a test program inside QEMU
- Evidence you used `argint`/`argaddr`/`copyout` correctly, not raw pointer
  dereferences on user memory

### What's NOT expected

- Scheduler or memory-management changes — that's a later week
- A production-quality implementation; a minimal, correct one is enough
- All four problems — Problems 1 and 2 are the minimum

### File hygiene

Submit your test programs and the specific files you changed — not the
entire `xv6-riscv` tree.

---

## Stuck?

- **Kernel won't build after your change?** Read the compiler error
  carefully — a missing semicolon in `syscall.h` or a mismatched function
  signature in `sysproc.c` are the most common causes.
- **`make` succeeds but your user program fails to link?** You almost
  certainly forgot the `usys.pl` entry — the kernel-side wiring is fine,
  but there's no user-space stub for your syscall to call.
- **Your syscall always returns garbage or `-1` from the wrong place?**
  Add a `printf` inside your `sys_*` function as a first move — confirm
  it's even being reached before debugging further.
- **`panic` inside QEMU?** Read the panic message; it names the file and
  line. A common cause this week is a missing `acquire`/`release` pair, or
  walking off the end of the `proc[]` array.
- Re-read `theory.md`, Section 4, if anything involving `copyout` or user
  pointers is confusing — this is genuinely the hardest idea introduced
  this week.
- Still stuck? Message your mentor.
