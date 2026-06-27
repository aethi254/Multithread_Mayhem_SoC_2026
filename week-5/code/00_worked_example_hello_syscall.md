# Worked Example: Adding `hello()` — a syscall with no arguments

This walks through adding one complete, trivial system call from scratch,
touching every one of the five places listed in Section 5 of `theory.md`.
`hello()` takes no arguments and just returns the fixed value `42` — it
exists purely so you can see the whole pattern once, end to end, before
you build something that actually does work (Problems 2–4).

**Do this in your own `xv6-riscv` checkout, and build/test it, before
starting the problem set.** The syscalls you'll add in the problems are
different from this one — don't just copy these diffs.

---

## 1. `kernel/syscall.h` — pick a number

Open the file and find the highest existing `SYS_*` number. In a stock
checkout that's `SYS_close`. Add one past it:

```c
#define SYS_close   21
#define SYS_hello   22   // <-- new
```

If you've already added a syscall from an earlier problem, use the next
number after *that* one instead — numbers must be unique and contiguous
isn't required, but skipping one means a wasted (but harmless) array slot.

---

## 2. `user/user.h` — declare it for user programs

Find the block of syscall prototypes (near the other `int fork(void);`
style declarations) and add:

```c
int hello(void);
```

This is the only reason a user `.c` file can write `hello()` and have it
compile — without this prototype, the compiler has no idea such a function
exists.

---

## 3. `user/usys.pl` — generate the user-space stub

This script generates `user/usys.S` at build time. Find the list of
`entry("...")` calls near the bottom and add one:

```perl
entry("close");
entry("hello");   # <-- new
```

After your next `make`, `user/usys.S` will contain a freshly generated
stub that looks like this (you don't write this by hand — it's generated):

```asm
.global hello
hello:
 li a7, SYS_hello
 ecall
 ret
```

This is the file described in theory Section 2, Step 1 — register `a7`
gets the syscall number, then `ecall` traps into the kernel.

---

## 4. `kernel/syscall.c` — wire it into the dispatch table

Two additions. First, near the other `extern` declarations:

```c
extern uint64 sys_close(void);
extern uint64 sys_hello(void);   // <-- new
```

Then, inside the `syscalls[]` array:

```c
static uint64 (*syscalls[])(void) = {
[SYS_fork]    sys_fork,
...
[SYS_close]   sys_close,
[SYS_hello]   sys_hello,   // <-- new
};
```

This is the table `syscall()` indexes into using the number it read out of
`a7` — see theory Section 3.

---

## 5. `kernel/sysproc.c` — the actual implementation

Add the function itself anywhere in the file (near the other simple
`sys_*` functions like `sys_getpid` is a good spot):

```c
uint64
sys_hello(void)
{
  printf("hello() was called from the kernel side!\n");
  return 42;
}
```

Notice the signature: `uint64 sys_hello(void)` — **no parameters**, because
`hello()` doesn't take any arguments. If it did, you'd read them out of the
trapframe using `argint`/`argaddr`/`argstr` as described in theory Section
4, not as C function parameters.

---

## Build and test it

```bash
cd xv6-riscv
make clean && make && make qemu
```

A clean rebuild is important here — you touched kernel headers, and stale
object files are a common source of "it's not working" confusion that has
nothing to do with your actual code.

Write a tiny test program, `user/hello_test.c`:

```c
#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int ret = hello();
  printf("hello() returned %d\n", ret);
  exit(0);
}
```

Add it to `UPROGS` in the Makefile (`$U/_hello_test\`), rebuild, and run it
inside QEMU:

```
$ hello_test
hello() was called from the kernel side!
hello() returned 42
```

The first line is printed by the **kernel** (`printf` inside `sys_hello`
goes to the console directly, same as any other kernel `printf`). The
second line is printed by the **user program**, after the value crossed
back through the trapframe's `a0`, through `usertrapret`/`userret`/`sret`,
and arrived back as the ordinary-looking return value of a C function
call.

If you see both lines, you've successfully added a system call to a real
operating system kernel. Everything in the problem set this week is a
variation on exactly these five steps.
