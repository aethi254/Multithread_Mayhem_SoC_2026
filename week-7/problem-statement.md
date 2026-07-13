# Week 7: Problem Set — Thread Design

This week you extend `struct proc` and build the pieces `thread_create()`/`thread_join()` will need next week — without writing either of those functions yet. Every problem below (except Problem 1, which is a design/trace exercise) requires editing files inside `kernel/`.

**Read `theory.md` in full before starting, especially Sections 3–6.** Problems 2–4 will not make sense without them.

---

## Overview

| # | Problem | Focus | Required? |
|---|---|---|---|
| 01 | Address-space design trace | Reading `uvmcopy()` and designing what "share instead of copy" would need | Yes |
| 02 | Thread-group bookkeeping | Add `tgid`/`is_thread` to `struct proc` + a read-only syscall | Yes |
| 03 | Thread stack allocator | A syscall that carves out a fresh user stack in the current address space | Recommended |
| 04 | Guard pages | Harden the allocator from Problem 3 and deliberately trigger the fault | Stretch |

Problems 1 and 2 are the minimum. Problem 3 is strongly encouraged — it's the piece Week 8's `thread_create()` will call directly. Problem 4 is optional but is the one that most directly tests whether you understood Section 6 of the theory.

---

## Setup

You're reusing the same xv6 checkout from previous weeks. No new toolchain steps.

```bash
cd xv6-riscv
make clean && make && make qemu
```

Before touching any kernel file, read the worked example at
`code/00_worked_example_am_i_a_thread.md`. It adds a tiny read-only syscall, `am_i_a_thread()`, that reports one bit of thread-group state end-to-end. Follow along in your own checkout, build it, and call it from a throwaway test program before starting Problem 2. The problems below assume you've done this once already and won't re-explain the five touch points from Week 5.

---

## Problem 01 — Address-space design trace

**WARM-UP · REQUIRED**

This problem requires no new code. It's a source-reading and design exercise: understand exactly what `fork()` does to set up a child's address space, then design — on paper, in words — what would need to change to make two `struct proc`s *share* one instead.

### What to do

1. Open `kernel/vm.c` and find `uvmcopy()`. Read it line by line. For each mapped page in the parent's range `[0, sz)`, what three things does it do? Cite the specific lines that (a) allocate a new physical page, (b) copy the bytes, (c) install the new PTE in the child's page table.
2. Open `kernel/proc.c` and find where `fork()` calls `uvmcopy()`. What does `fork()` do if `uvmcopy()` fails partway through (returns a negative value)? What does this tell you about who owns cleanup responsibility when a multi-step kernel operation fails midway?
3. **Design question — no xv6 source will answer this directly, this is you designing:** if you wanted `fork()` (or a new function next to it) to produce a child that *shares* the parent's page table instead of copying it, what would you do differently at each of these three points?
   - Where `uvmcopy()` is currently called
   - How you'd know when it's safe to actually free the page table (hint: re-read theory Section 4 on refcounting)
   - What `p->sz` should be for the sharing child, and whether `p->pagetable` should be a distinct pointer value or the literal same pointer as the parent's
4. One more design question: this week's Problem 3 allocates thread stacks by growing `p->sz` upward, the same direction `sbrk()` grows the heap. If two threads shared a page table but each had their *own* `p->sz` (rather than a shared one), what could go wrong? (Hint: think about what `p->sz` is used for elsewhere — argument validation in `copyin`/`copyout`, Week 5.)

### What to submit

A markdown or plain-text file, `thread_design.md`, with one section per numbered item above. For items 1–2, cite actual file names and approximate line numbers from your own checkout. For items 3–4, write your reasoning in your own words — there is no single correct answer expected, but a vague or hand-wavy answer ("it would just share it") is not sufficient; explain the *mechanism*.

---

## Problem 02 — Thread-group bookkeeping

**REQUIRED**

Add the two fields described in theory Section 3 to `struct proc`, initialize them correctly, and add a read-only system call, `getthreadinfo()`, that reports them (plus the calling process's own pid) to user space.

### What to do

1. Add to `kernel/proc.h`:
   ```c
   int is_thread;   // 0 = ordinary process, 1 = a thread (Week 8 will set this)
   int tgid;        // thread-group id: pid of the proc that "owns" the shared address space
   ```
2. In `allocproc()` (`kernel/proc.c`), initialize both fields for every newly allocated `struct proc`:
   - `p->is_thread = 0;`
   - `p->tgid = p->pid;` — **note:** `p->pid` is assigned earlier in `allocproc()`, by the time you reach the point where other fields like `p->state` are set up; make sure you initialize `tgid` *after* `p->pid` has its final value, not before.
3. Add a user-visible struct in `user/user.h`:
   ```c
   struct thread_info {
     int pid;
     int tgid;
     int is_thread;
   };

   int getthreadinfo(struct thread_info *info);
   ```
4. Implement `sys_getthreadinfo()` in `kernel/sysproc.c`. It takes one pointer argument (use `argaddr`, same pattern as Week 5's `getcmdline` and Week 6's `schedinfo`), fills in a local `struct thread_info` from `myproc()`, and `copyout`s it to the user-supplied address. Return `0` on success, `-1` if the copy fails.
5. Wire through the usual five touch points (`syscall.h`, `user.h`, `usys.pl`, `syscall.c`, `sysproc.c`).

### A question to answer in your report

For an ordinary process created by plain `fork()` (no threads involved anywhere), what should `getthreadinfo()` report for `tgid` relative to `pid`? Confirm your test output actually shows this.

### Test it

Write `threadinfo_test.c` that calls `getthreadinfo()` once and prints all three fields. Then `fork()` once, and have **both** parent and child call it again, printing which one they are (`"parent"` / `"child"`) alongside the result — confirm the child gets its own, distinct `pid` and `tgid` (equal to each other, since no thread-sharing exists yet), not a copy of the parent's.

### What to submit

Your kernel diffs for `kernel/proc.h`, `kernel/proc.c`, `kernel/syscall.h`, `kernel/syscall.c`, `kernel/sysproc.c`, `user/user.h`, and your `user/usys.pl` entry, plus `threadinfo_test.c` and your answer to the question above in your report.

---

## Problem 03 — Thread stack allocator

**RECOMMENDED**

Add a system call, `alloc_thread_stack(int size)`, that maps a fresh, previously-unused region of `size` bytes into the **calling process's own address space** — above whatever it currently has mapped — and returns the address that should become a new thread's initial stack pointer (since stacks grow down, this is the **top**, i.e. highest address, of the newly mapped region). Return `-1` if `size <= 0` or if the underlying allocation fails.

### What to do

This is a smaller, more targeted version of what `sys_sbrk()` already does — you're deliberately reusing the same primitive `growproc()` is built on, not inventing a new one.

1. Add the five touch points for `alloc_thread_stack(int size)`.
2. In `sys_alloc_thread_stack()` (`kernel/sysproc.c`):
   - `argint(0, &size)` to fetch the requested size; return `-1` immediately if `size <= 0`.
   - Round `size` up to a whole number of pages with `PGROUNDUP` (`kernel/riscv.h`).
   - Call `uvmalloc(p->pagetable, p->sz, p->sz + size, PTE_W)` — this is the exact function `growproc()` calls for `sbrk()`. Read `uvmalloc()` in `kernel/vm.c` before calling it if you haven't already (Problem 1 should have taken you through it).
   - If `uvmalloc()` returns `0`, the allocation failed — return `-1` without changing `p->sz`.
   - Otherwise, update `p->sz` to the new size it returned, and return that same value (the new `p->sz` **is** the top of the freshly mapped region, exactly the stack-pointer value a new thread would need).

### A known limitation (discuss, don't fix, in this problem)

This version has **no guard page** — two consecutive calls will produce two stacks that sit directly adjacent in memory, with no protection between them. That's fine for this problem; Problem 4 fixes it. For now, just notice it and be ready to explain in your report what could go wrong if a thread using one of these stacks overflowed it.

### Test it

Write `stack_alloc_test.c` that:

- Calls `alloc_thread_stack(4096)` and prints the returned address.
- Calls it again with the same size and prints the second returned address.
- Prints the difference between the two returned addresses and confirms it's exactly `4096` — i.e., the stacks are perfectly adjacent, with the limitation above visibly demonstrated.

### What to submit

Your kernel diffs and `stack_alloc_test.c`, plus a note in your report about what you observed and what could go wrong with the no-guard-page limitation.

---

## Problem 04 — Guard pages

**STRETCH**

Harden the allocator from Problem 3 by leaving one unmapped **guard page** immediately below each newly allocated stack region, and write a test program that deliberately overflows into it to observe the resulting page fault.

### What to do

1. Add a second syscall, `alloc_thread_stack_guarded(int size)` (you can leave Problem 3's version in place unchanged, or have this one replace it — your choice, document which).
2. In `sys_alloc_thread_stack_guarded()`:
   - Same argument handling and rounding as Problem 3.
   - Call `uvmalloc(p->pagetable, p->sz, p->sz + PGSIZE + size, PTE_W)` — note the extra `PGSIZE`: you're mapping one *extra* page beyond what's usable, which will become the guard.
   - If that succeeds, immediately call `uvmunmap(p->pagetable, oldsz, 1, 1)` — this unmaps (and frees the physical page backing) exactly the **lowest** page of the region you just mapped, i.e. `oldsz`, the address right where the old `p->sz` was. `do_free = 1` means the physical page is returned to the free list, since nothing should ever legitimately be stored there.
   - Set `p->sz` to the new (higher) size, and return it — the top of the *usable* part of the stack.
3. The net effect: `[old p->sz, old p->sz + PGSIZE)` is a hole (unmapped guard page), and `[old p->sz + PGSIZE, new p->sz)` is the actual usable stack, with its top being the returned value.

### A question to answer in your report

If a thread's stack pointer starts at the returned top and grows downward, how many bytes of *actual, usable* stack does it have before it hits the guard page? (It's not simply the `size` you asked for — work out the exact relationship to `PGSIZE` and explain it.)

### Test it

Write `guardpage_test.c` that:

1. Calls `alloc_thread_stack_guarded(4096)`, prints the returned top address.
2. Writes a test byte to `top - 1` (the last valid byte of the usable stack) and prints confirmation that it succeeded.
3. **Deliberately** writes a test byte to an address `size` bytes *and one more* below `top` — i.e., one byte inside the guard page — and prints a message right before doing so.
4. Does **not** print anything after step 3, because the process should not survive it.

Run it inside QEMU and observe what xv6 prints when the fault happens. It will not look like your own program's output — it will look like a kernel diagnostic. Copy that message into your report.

### What to submit

Your kernel diffs, `guardpage_test.c`, the actual console output you observed (including the kernel's fault message), and your answer to the usable-bytes question above.

---

## Submission

Keep ready for the Google Form your mentor will share:

| Problem | File(s) to keep ready |
|---|---|
| Problem 1 — Address-space design trace | `thread_design.md` |
| Problem 2 — Thread-group bookkeeping | Kernel diffs + `threadinfo_test.c` |
| Problem 3 — Thread stack allocator | Kernel diffs + `stack_alloc_test.c` |
| Problem 4 — Guard pages | Kernel diffs + `guardpage_test.c` |

"Kernel diffs" means: either `git diff` output from your xv6 checkout, or the full modified files (`proc.h`, `proc.c`, `syscall.h`, `syscall.c`, `sysproc.c`, `usys.pl`, `user.h`) — whichever your mentor's form asks for. Don't submit the entire xv6 tree.

Also keep ready a short **report** covering:

- Which problems you attempted
- Your design answers from Problem 1
- Your answer to the `tgid`-vs-`pid` question in Problem 2
- What you observed from `stack_alloc_test` in Problem 3
- (If attempted) the guard-page fault message and usable-bytes answer from Problem 4
- One thing that surprised you
- Anything you got stuck on

### What's expected

- A clean `make clean && make` with no new warnings
- A working xv6 that boots after your `struct proc` and allocator changes
- Test programs whose output you can explain, not just reproduce
- Evidence you read `uvmcopy()`, `uvmalloc()`, and `uvmunmap()` in the actual source — cite specific files and line numbers in Problem 1

### What's NOT expected

- A working `thread_create()` or `thread_join()` — that's all of Week 8
- Any scheduler changes — Week 6's scheduler is untouched this week
- A general-purpose, freeable stack allocator — grow-only is fine
- All four problems. Problems 1 and 2 are the minimum

### File hygiene

Submit your test programs and the specific files you changed — not the entire `xv6-riscv` tree.

---

## Stuck?

- **`make` fails after adding fields to `struct proc`?** Check you added them inside the struct body, not after its closing brace, and that you didn't duplicate a field name already in use.
- **`tgid` prints as `0` for every process?** You likely initialized it before `p->pid` was assigned its final value in `allocproc()` — check the ordering.
- **`getthreadinfo` returns `-1` every time?** Almost always a `copyout` size mismatch — double check you're passing `sizeof(info)` where `info` is the actual struct, not a pointer to it.
- **Your two stacks in Problem 3 don't come out exactly 4096 apart?** You're probably not using the *return value* of `uvmalloc()` (which reflects actual mapped size, page-rounded) as the new `p->sz` — recompute from that, not from your own arithmetic on the original `size`.
- **Problem 4's guard page fault never happens?** Print the exact address you're writing to in your test program and the exact range `uvmunmap` was called on — an off-by-one in either place means you're still writing into mapped memory.
- **`panic: uvmunmap: not mapped`?** You called `uvmunmap` on an address that wasn't actually mapped by the preceding `uvmalloc` — check your page-rounding arithmetic before that call, not after.
- Re-read `theory.md` Section 5 if you find yourself wanting to change the scheduler this week — you shouldn't need to, for anything in this problem set.
- Still stuck? Message your mentor.
