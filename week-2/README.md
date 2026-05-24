# Week 2 — Synchronization

> Goal of the week: take the broken programs from Week 1, understand exactly why they broke, and fix them properly using the synchronization primitives that real production software is built on.

Last week you wrote programs that misbehaved on purpose. This week you build the tools that make concurrent code actually work — mutexes, condition variables, and the patterns that wrap them. By the end of the week you'll be able to write thread-safe data structures and producer-consumer pipelines, and you'll recognize the new class of bugs (deadlocks) you can create while trying to fix the old ones.

---

## What you'll know by the end of the week

- Why **mutual exclusion** is the right abstraction for protecting shared data, and what a lock is conceptually
- How to use the **pthread mutex API**: `pthread_mutex_lock`, `unlock`, `trylock`, `init`, `destroy`
- The difference between **coarse-grained** and **fine-grained** locking, and when to use each
- How **condition variables** let threads wait for events instead of burning CPU on spin-waits
- The **producer-consumer** pattern — the workhorse of concurrent programming
- What a **deadlock** is, the four conditions required for one, and how lock ordering prevents them
- A working knowledge of related bugs: livelock, starvation, priority inversion

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
| 3 | Read section 4 of `theory.pdf`. Run code example 04 and watch a real deadlock happen. |
| 4 | Start the problem set. |
| 5 | Finish the problem set. Get your report ready for submission. |

Read, *then* run, *then* solve — same as last week.

---

## How to compile and run the code

Inside `code/` there's a `Makefile` that builds everything:

```bash
cd week-2/code
make             # builds all four examples
./counter_fixed  # run the fixed counter
./atomic_counter # run the atomic version
./producer_consumer
./deadlock_demo  # WARNING: this one hangs on purpose — Ctrl+C to kill it
make run-fixed   # builds and runs the working examples
make clean       # removes compiled binaries
```

The `deadlock_demo` is excluded from `run-all` because it deliberately hangs forever — running it via `make run-all` would never finish.

---

## A word of warning before you begin

You are now fixing real bugs. Two new failure modes to watch for:

1. **You forgot to unlock.** Your program will hang because the next thread that tries to lock will wait forever. Always pair every `lock` with an `unlock`, including in error paths.
2. **You introduced a deadlock.** Two threads grab two locks in opposite orders. Now nothing makes progress. The program just sits there. You will write one of these on purpose in problem 4 — recognize the symptom now so you don't get caught by it in problems 1–3.

If your program ever just *stops* and you're sure it should be running, your first suspicion should be a lock you didn't release or a deadlock you didn't see.

---

## Submission instructions

Submissions for this week will be collected via a **Google Form**. Your mentor will share the form link separately.

You do not need to fork the repo, push to GitHub, or open any pull requests. Just have your files ready and fill the form when the link drops.

### Files to keep ready

| Problem | File to keep ready |
|---|---|
| Problem 1 — Fix last week's mess | `parallel_sum_fixed.c`, `bank_chaos_fixed.c` |
| Problem 2 — Thread-safe bounded buffer | `bounded_buffer.c` |
| Problem 3 — Thread-safe linked list *(if attempted)* | `safe_list.c` |
| Problem 4 — Dining philosophers *(if attempted)* | `philosophers.c` (and your write-up of the deadlock scenario you saw) |

Also keep ready a short **report** — half a page is plenty — covering:

- Which problems you attempted
- For problem 1: the speed / correctness of the fixed versions vs the originals
- For problem 2: how you tested the bounded buffer and what edge cases you considered
- (If attempted) results for problems 3 and 4
- One thing that surprised you this week
- Anything you got stuck on

You'll be able to paste this report directly into the form, so it can live in a plain text file, a Google Doc, or just a draft in your notes app — whatever you prefer.

### What's expected

- Code that compiles cleanly with `gcc -Wall -pthread`. Warnings should be fixed, not silenced.
- Every program **terminates** when run normally. No accidental deadlocks in your submitted code (problem 4's intentional ones are obviously fine).
- Run each program multiple times to convince yourself the race is actually gone. A program that produces the right answer once is not the same as a program that always produces the right answer.

### What's NOT expected

- A perfectly fine-grained lock design. A single big lock per data structure is fine for this week.
- Performance benchmarks. Save those for Week 4.
- All four problems. Problems 1 and 2 are the minimum.

### File hygiene

Do not submit compiled binaries — only `.c` source files. If a problem has multiple files, zip them together before uploading.

---

## Stuck?

- Re-read the relevant section of `theory.pdf`. Often the answer is there.
- Compare your output to `sample-outputs/` for the example you're running.
- If your program hangs: it's almost certainly an unreleased lock or a deadlock. Add print statements around your `lock`/`unlock` calls to see which thread is stuck where.
- If you're still stuck, open an issue or message your mentor.
