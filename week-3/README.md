# Week 3 — Pthreads API & Classic Problems

> Goal of the week: learn the full pthreads API beyond the basics, understand how threads return results, meet semaphores as a third synchronization primitive, and tackle one of the most famous coordination problems in computer science.

Weeks 1 and 2 gave you the tools. This week you use them properly. You'll go deeper into the API you've been using — thread lifecycle, attributes, how to pass data in and get data back — and then add semaphores to your toolkit. The week closes with the Readers-Writers problem, which introduces a pattern (multiple concurrent readers, single exclusive writer) that shows up in every database, web server, and file system you will ever use.

---

## What you'll know by the end of the week

- The **full thread lifecycle** — the five states a thread moves through, from creation to termination
- How to configure threads at creation time using **`pthread_attr_t`**
- The difference between **joinable** and **detached** threads, and when to use each
- How to **return data from a thread** safely, without dangling pointers
- How to use **POSIX semaphores** (`sem_init`, `sem_wait`, `sem_post`) and how they differ from mutexes
- The **Readers-Writers problem**, its two classic variants, and the `pthread_rwlock_t` solution

---

## What's in this folder

| File / folder | What it is |
|---|---|
| `theory.pdf` | All the concepts for the week. Read this first. |
| `problem-statement.pdf` | The four problems you'll work on. |
| `code/` | Four runnable example programs referenced from `theory.pdf` and the problems. |
| `sample-outputs/` | Reference outputs from running each program. Compare with your own runs. |

---

## Suggested plan for the week

| Day | What to do |
|-----|------------|
| 1 | Read sections 1 and 2 of `theory.pdf`. Compile and run code examples 01 and 02. |
| 2 | Read section 3 of `theory.pdf`. Run code example 03. |
| 3 | Read section 4 of `theory.pdf`. Run code example 04. |
| 4 | Start the problem set. |
| 5 | Finish the problem set. Get your report ready for submission. |

Read, *then* run, *then* solve — same as the previous weeks.

---

## How to compile and run the code

Inside `code/` there's a `Makefile` that builds everything:

```bash
cd week-3/code
make               # builds all four examples
./thread_lifecycle
./return_values
./semaphores
./readers_writers
make run-all       # builds and runs all four in order
make clean         # removes compiled binaries
```

All programs terminate cleanly. Compile flag required: `-pthread -std=c11`.

Or manually:

```bash
gcc -Wall -pthread -std=c11 01_thread_lifecycle.c -o thread_lifecycle
```

---

## A word of warning before you begin

Two new things that will bite you this week:

**Detached threads and early exit.** If `main` returns while detached threads are still running, the whole process is torn down and they die silently. If your detached-thread output seems to be cut off, that's why. Use a `sleep` or a semaphore to let them finish.

**Returning stack pointers from threads.** The single most common Week 3 mistake is returning a pointer to a local variable from a thread function. The local variable is gone the moment the thread returns. The pointer is now dangling. The code will sometimes work (the memory hasn't been overwritten yet), which makes the bug extra hard to spot. Always return heap-allocated data from threads.

---

## Submission instructions

Submissions for this week will be collected via a **Google Form**. Your mentor will share the form link separately.

You do not need to fork the repo, push to GitHub, or open any pull requests. Just have your files ready and fill the form when the link drops.

### Files to keep ready

| Problem | File to keep ready |
|---|---|
| Problem 1 — Parallel statistics | `parallel_stats.c` |
| Problem 2 — Semaphore bounded buffer | `sem_buffer.c` |
| Problem 3 — Readers-writers from scratch *(if attempted)* | `rw_lock.c` |
| Problem 4 — Thread pool *(if attempted)* | `thread_pool.c` |

Also keep ready a short **report** — half a page is plenty — covering:

- Which problems you attempted
- For problem 1: which stats you computed, how you collected thread return values, and whether results were always correct
- For problem 2: how your semaphore-based version differs from the Week 2 condvar version
- *(If attempted)* results for problems 3 and 4
- One thing that surprised you this week
- Anything you got stuck on

You'll be able to paste this report directly into the form, so it can live in a plain text file, a Google Doc, or just a draft in your notes app — whatever you prefer.

### What's expected

- Code that compiles cleanly with `gcc -Wall -pthread -std=c11`. Warnings should be fixed, not silenced.
- Every program terminates when run normally.
- Honest observations — if your parallel version was unexpectedly slower than sequential, say so and think about why.

### What's NOT expected

- A production-quality thread pool. A minimal implementation that works is fine for problem 4.
- Familiarity with named semaphores (`sem_open`) — we only use unnamed semaphores this week.
- All four problems. Problems 1 and 2 are the minimum.

### File hygiene

Do not submit compiled binaries — only `.c` source files. If a problem has multiple files, zip them together before uploading.

---

## Stuck?

- Re-read the relevant section of `theory.pdf`. Often the answer is there.
- Compare your output to `sample-outputs/` for the example you're running.
- If a thread isn't terminating: check for a missing `sem_post`, an unreleased lock, or a detached thread that outlived `main`.
- If you're still stuck, open an issue or message your mentor.
