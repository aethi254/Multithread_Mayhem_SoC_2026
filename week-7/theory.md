# Week 7 Theory: Thread Design — Structure, Stacks, Context Switching

> Weeks 4–6 built you a working process abstraction on top of xv6: address spaces, system calls, a scheduler, locks. This week you stop adding new machinery and instead ask a sharper question about the machinery you already have: **exactly what does a thread need that a process doesn't already give it for free?** Answering that precisely is the whole point of this week — Week 8 is just wiring up the answer.

---

## 1. Recap: What `fork()` Actually Gives You

When a user program calls `fork()`, the xv6 kernel (`kernel/proc.c`) does roughly this:

1. `allocproc()` finds a free `struct proc` slot.
2. `uvmcopy()` (`kernel/vm.c`) walks the **parent's** page table from address `0` to `p->sz`, and for every mapped page:
   - allocates a **new physical page** with `kalloc()`
   - copies the parent page's bytes into it
   - installs a **new PTE** in the **child's** page table pointing at the new physical page, with the same permission bits as the parent's PTE
3. File descriptors are duplicated (same open files, shared offset via the underlying `struct file`).
4. The child's trapframe is copied from the parent's, except `a0` is set to `0` (so `fork()` returns `0` in the child).
5. `p->state = RUNNABLE` and the child is now eligible to be scheduled.

The important detail: **every mapped page gets its own physical copy.** Two processes created by `fork()` share nothing about their memory after the call returns — if the child writes to a byte, the parent's corresponding byte is completely unaffected, because they are backed by different physical pages. This is exactly what you want for two unrelated processes. It is exactly what you *don't* want for two threads of the same program, which are supposed to see each other's writes to shared variables immediately.

---

## 2. What a Thread Actually Needs

A thread, at minimum, needs three things that are separate from every other thread in the same group:

| Needs its own | Why |
|---|---|
| **Program counter** | Each thread executes different code at any given moment |
| **Registers / stack pointer** | Each thread has its own local variables, call frames, return addresses |
| **A `struct proc` slot** (in this project's design) | xv6's scheduler operates on `struct proc` — to be independently schedulable, a thread needs to *be* one |

And it needs to **share** with its sibling threads:

| Shares | Why |
|---|---|
| **The page table / address space** | So writes by one thread are visible to all others — this is the entire point of threading |
| **Open file descriptors** | Threads of the same program typically operate on the same files |
| **Global/heap memory** | Any `malloc`'d or global data must be visible everywhere |

Look at that first table again: PC, registers, and stack pointer are *already* exactly what `struct context` and `struct trapframe` capture per-`struct proc` (Week 6, Section 4). And a `struct proc` slot is exactly the unit the scheduler already round-robins over. **xv6 already has almost everything a thread needs — it's built for processes, and a thread is 90% of a process.** The only field that's currently *wrong* for a thread is the page table: `fork()`'s `uvmcopy()` gives every process a private copy, and a thread needs to share one instead.

This project's design choice, used by several teaching operating systems built on xv6, is exactly that: **implement a thread as a `struct proc` that shares its page table with its siblings instead of getting a private copy.** Nothing about the scheduler, `swtch`, or the trapframe machinery needs to know the difference.

---

## 3. Thread-Group Bookkeeping

If two `struct proc` slots share a page table, the kernel needs a way to know that — both so it can decide *not* to tear down the page table when just one of the threads exits (Section 4), and so a future `thread_join()` can find "the other threads that belong with me" instead of only its `fork()`-children.

This week you add two small fields to `struct proc` (`kernel/proc.h`):

```c
struct proc {
  ...
  int is_thread;   // 0 = ordinary process / the group's "main" thread, 1 = a thread sharing another proc's address space
  int tgid;        // "thread group id" - the pid of the proc that owns the shared page table
  ...
};
```

Convention used in this design:

- An ordinary process created by `fork()` has `is_thread = 0` and `tgid = p->pid` (it is the sole member, and the "leader," of its own one-member group).
- A thread created by `thread_create()` (Week 8) will have `is_thread = 1` and `tgid` set to the pid of the process it was created from.
- `getprocstate()` (Week 5) and `is_lock_held()` (Week 6) don't need to know about any of this — they operate below the level of threads vs. processes. `wait()`, on the other hand, will need to change in Week 8 so a thread's exit doesn't get reaped the same way a child process's does — but that change is out of scope this week. This week, you only add the fields and a read-only syscall to inspect them (Problem 2).

---

## 4. Sharing a Page Table Safely: The Refcounting Problem

Suppose two `struct proc` slots, A and B, point at the *same* `pagetable_t`. Now suppose A calls `exit()`. Ordinarily, `exit()` (via `freeproc()`) calls `proc_freepagetable()`, which walks the page table, frees every physical page it points at, and frees the page-table pages themselves. If A does that while B is still running, B's next memory access walks a page table that no longer exists — instant, silent corruption or a kernel panic, and not even a useful one.

The general fix (which you are **not** required to implement this week — it's previewed here because Week 8 needs it and the reasoning matters now) is a **reference count** attached to the shared page table: increment it whenever a new thread starts sharing it, decrement it whenever a thread holding it exits, and only actually call `proc_freepagetable()` when the count reaches zero. This is conceptually the same pattern as the reference-counted `struct file` you saw briefly in Week 4 — multiple owners, and the last one out turns off the lights.

**Write this down for your report in Problem 1:** where would you store this counter? Two reasonable answers: (a) a small heap-allocated `int` (allocated with `kalloc()`, which conveniently hands out whole pages — wasteful but simple) pointed to by every sharing `struct proc`, incremented/decremented under a lock; or (b) a field alongside the pagetable itself if xv6's pagetable representation is wrapped in your own struct. Either is fine; the property that matters is that the increment/decrement pair is atomic with respect to `exit()` racing another thread's `exit()`.

---

## 5. Context Switching Needs No Changes At All

This is the single most important idea this week, and it's good news: **everything you built in Week 6 — `scheduler()`, `sched()`, `swtch.S`, spinlocks, sleep/wakeup — works completely unmodified for threads.**

Walk through why:

- `scheduler()` picks any `RUNNABLE` `struct proc` and calls `swtch(&c->context, &p->context)`. It does not look at `p->pagetable` to decide whether to schedule a process — it only checks `p->state`. A thread's `struct proc` looks, from the scheduler's point of view, exactly like any other process's.
- `swtch` (`kernel/swtch.S`) saves and restores 14 callee-saved registers, full stop. It has never known or cared what address space a process runs in — the page table is switched separately, by loading `satp` from `p->pagetable` when the process actually returns to user mode (in the trampoline's `userret`), not inside `swtch` itself.
- Two threads sharing a page table simply means their `satp` values happen to be numerically identical. `userret` doesn't need to know or care that this is the *same* `satp` it loaded a moment ago for a sibling thread — it reloads it unconditionally either way.

**The only two things that change to support threads, then, are:**

1. **How the page table is set up at creation time** — share a pointer (and bump a refcount) instead of calling `uvmcopy()` to make a private copy. (Week 8.)
2. **Where the new thread's initial stack pointer points** — since it can't reuse the address range of an existing thread's stack, it needs a *freshly allocated, private* region within the *shared* address space. This week's Problem 3 builds exactly that allocator.

If you finish this week and still feel like the scheduler needs to change somehow to support threads, that's a sign to re-read this section — it's a common, understandable, and wrong intuition. The scheduler was already generic enough.

---

## 6. Stacks: Why Sharing an Address Space Doesn't Mean Sharing a Stack

Even though thread siblings share one address space, each thread absolutely needs its **own user stack**. If two threads used the same stack region, their local variables and return addresses would collide the instant both were active — this isn't a subtle bug, it's immediate and total corruption.

The fix is straightforward: carve out a *new*, previously-unused range of virtual addresses within the shared address space for each new thread's stack, the same way `sbrk()` grows a process's heap by asking `uvmalloc()` (`kernel/vm.c`) to map fresh physical pages at the next unused virtual addresses (`p->sz` upward). Problem 3 has you write exactly this: a syscall that calls `uvmalloc()` to map `N` fresh pages above the process's current `sz`, and hands back the address that should become the new thread's initial stack pointer.

### Guard pages

An unguarded stack allocation has a sharp edge: if a thread's stack pointer marches downward past the bottom of its allocated region (a runaway recursion, an oversized local array), it silently starts corrupting whatever happens to be mapped at the next-lower address — which, depending on allocation order, might be *another thread's stack*. That's a data race and a memory-safety bug at the same time, and it can be very hard to debug because the symptom shows up in a completely different thread than the one that caused it.

A **guard page** fixes this cheaply: leave one page, immediately below the usable stack region, **unmapped**. If a thread's stack pointer (and any write through it) ever reaches that page, the hardware raises a page fault instead of silently succeeding against whatever physical page happened to be there. xv6's `usertrap()` (`kernel/trap.c`) doesn't have a stack-growth handler (some real kernels do — this is exactly what a segfault-into-stack-growth mechanism looks like in Linux, minus the auto-growth part), so the fault is fatal to that one process: `usertrap()` prints a diagnostic naming the faulting `scause`, sets `p->killed = 1`, and the process exits. The rest of the system — other processes, other threads, the shell — is completely unaffected. Problem 4 has you build this and deliberately trigger the fault to see it happen.

---

## 7. What Week 8 Builds On Top Of This

Everything above exists to make the following two functions almost mechanical to write next week:

- **`thread_create(void (*fn)(void*), void *arg)`** — allocates a new `struct proc` (like `fork()`'s `allocproc()`), but instead of `uvmcopy()`, **shares** the calling process's `pagetable` pointer and bumps its refcount; calls `alloc_thread_stack()` (this week's Problem 3, with a guard page from Problem 4) to get a fresh stack; sets the new trapframe's `epc` to `fn` and `sp` to the returned stack top, and its `a0` to `arg`; marks `is_thread = 1` and `tgid` to the parent's pid; sets `state = RUNNABLE`.
- **`thread_join(int tid)`** — behaves like `wait()`, but searches for a `struct proc` with a matching `tgid` and `pid == tid` rather than a `fork()`-child, and — critically — must **not** free the shared page table when reaping a thread unless the refcount says it was the last one.

If Sections 3–6 above make sense, both of those functions are now "combine pieces you already understand," not "learn something new." That's the goal of this week.

---

## 8. References

- [xv6-riscv book](https://pdos.csail.mit.edu/6.828/2025/xv6/book-riscv-rev4.pdf) — Chapter 3 (`uvmcopy`, page tables), Chapter 7 (scheduling, revisited from Week 6's perspective).
- [xv6-riscv source](https://github.com/mit-pdos/xv6-riscv) — `kernel/vm.c` (`uvmcopy`, `uvmalloc`, `uvmunmap`), `kernel/proc.c` (`allocproc`, `fork`, `freeproc`), `kernel/proc.h`.
- OSTEP, Chapter 26 ("Concurrency and Threads") — the general OS theory behind why threads share an address space, independent of xv6's specific implementation.
- OSTEP, Chapter 4 ("The Abstraction: The Process") Section on address spaces, for a refresher on what `uvmcopy` is copying in the first place.
