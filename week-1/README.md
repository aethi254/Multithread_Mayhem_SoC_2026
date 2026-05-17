# Week 1 — Threads & Concurrency Basics

> Goal of the week: understand what a thread actually *is*, run multiple of them in a single program, and watch with your own eyes how they corrupt shared data when nobody is watching.

This is the foundation week of Multithread Mayhem. Everything else in the project — synchronization, classic problems, xv6's kernel-level threads — depends on the intuition you build here. **We are not going to fix any of the bugs you cause this week.** That is next week's job. This week we cause them and learn to recognize them.

---

## What you'll know by the end of the week

- The difference between a **program**, a **process**, and a **thread**, and why threads exist
- The **memory layout** of a process — code, data, heap, stack — and which parts threads share
- What a **race condition** is, why it happens at the hardware level, and how to spot one
- How to use the basic pthreads calls: **`pthread_create`** and **`pthread_join`**
- How to compile and run a multithreaded C program

---

## What's in this folder

| File / folder | What it is |
|---|---|
| `theory.pdf` | All the concepts for the week. Read this first — it assumes no OS background. |
| `problem-statement.pdf` | The four problems you'll work on. |
| `code/` | The four runnable example programs referenced from `theory.pdf` and the problems. |
| `sample-outputs/` | Reference outputs from running each program. Use these to compare with your own runs. |

---

## Suggested plan for the week

| Day | What to do |
|-----|------------|
| 1 | Read sections 1 and 2 of `theory.pdf`. Compile and run code examples 01 and 02. |
| 2 | Read section 3 of `theory.pdf`. Run code examples 03 and 04. |
| 3 | Read section 4 of `theory.pdf`. Start the problem set. |
| 4 | Finish the problem set. |
| 5 | Write your `REPORT.md` and submit. |

Adapt the schedule to your week — the point is: **read, *then* run, *then* solve**. Don't skip ahead to the problems before the examples. Half of the learning is in observing the example code misbehave.

---

## How to compile and run the code

Inside `code/` there's a `Makefile` that builds everything for you:

```bash
cd week-1/code
make             # builds all four examples
./hello_thread   # run the first example
./race_counter   # run the second example
./parallel_sum   # run the third example
./bank_chaos     # run the fourth example
make run-all     # builds and runs everything in sequence
make clean       # removes compiled binaries
```

If you'd rather compile manually:

```bash
gcc -Wall -pthread 01_hello_thread.c -o hello_thread
```

The `-pthread` flag is **important** — without it your program will not link correctly. `theory.pdf` explains why.

---

## A word of warning before you begin

Multithreaded programs are *non-deterministic*. The same program can produce different output every time you run it — and your bug might happen only 1 in 100 runs. **Always run the example programs multiple times** (10+). If something seems to "work correctly," try again with a larger loop count or on a different machine. The bug is there. Concurrency does not care whether you can see it.

That non-determinism is the hardest mental adjustment of this whole project. Get used to it now.

---

## Submission instructions

Submissions for this week will be collected via a **Google Form**. Your mentor will share the form link separately.

You do not need to fork the repo, push to GitHub, or open any pull requests. Just have your files ready and fill the form when the link drops.

### Files to keep ready

| Problem | File to keep ready |
|---|---|
| Problem 1 — Run, observe, write down | _no code file_ — your observations go in the report |
| Problem 2 — Make the race worse | `race_amplified.c` |
| Problem 3 — Parallel word count *(if attempted)* | `word_count.c` (and optionally `word_count_correct.c`) |
| Problem 4 — Stretch simulation *(if attempted)* | your simulation code, named descriptively (e.g. `ticket_booking.c`) |

Also keep ready a short **report** — half a page is plenty — covering:

- Which problems you attempted
- What you observed across 10+ runs of each example program (range of outputs, ever correct, how often)
- What you changed for problem 2 and why it makes the race more visible
- (If attempted) results for problems 3 and 4
- One thing that surprised you
- Anything you got stuck on

You'll be able to paste this report directly into the form, so it can live in a plain text file, a Google Doc, or just a draft in your notes app — whatever you prefer.

### What's expected

- Code that compiles cleanly with `gcc -Wall -pthread`. Warnings should be fixed, not silenced.
- Honest observations, not invented ones — if you didn't see a race on your machine, say so.
- Documentation of *multiple runs* for problems that involve concurrency.

### What's NOT expected

- Perfect, race-free code. This week is about *creating* races, not fixing them. Save the synchronization for Week 2.
- A lengthy writeup. Half a page is plenty.
- All four problems. Two is the minimum.

### File hygiene

Do not submit compiled binaries (`hello_thread`, `race_counter`, etc.) — only `.c` source files. If a problem has multiple files, zip them together before uploading.

---

## Stuck?

- Re-read the relevant section of `theory.pdf`. Often the answer is there.
- Compare your output to `sample-outputs/` for the example you're running.
- Check the GitHub Issues — someone may have hit the same thing.
- If you're still stuck, open an issue or message us. The point of this week is to *understand*, not to suffer in silence.