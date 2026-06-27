# Week 5 Theory: System Calls — Traps & Syscall Flow

> Last week you saw xv6 from the outside: you booted it, ran programs, and
> read about the user/kernel boundary in general terms. This week we open
> that boundary up and look at exactly what happens, instruction by
> instruction, when a user program asks the kernel to do something — and
> then you modify the kernel for the first time.

---

## 1. Recap: Crossing the Boundary

Recall from Week 4: your programs run in **U-mode**, the xv6 kernel runs in
**S-mode**, and the only legal way to move from one to the other is a
controlled crossing — the `ecall` instruction. You also learned the
high-level shape of that crossing: privilege rises, the program counter is
saved, `scause` records why the trap happened, and control jumps to a
handler address held in `stvec`.

That description was accurate but incomplete. It told you *that* a crossing
happens. This section traces *how* — concretely, in source files you can
open right now in your `xv6-riscv` checkout.

A **system call** is just a function call that happens to cross the
user/kernel boundary. From the calling program's point of view it looks
exactly like a normal C function call: `getpid()`, `sleep(10)`,
`write(1, buf, n)`. Everything described below is the machinery hiding
*underneath* that ordinary-looking function call.

---

## 2. The Trap Path in Detail

### Step 1 — The user-space stub

Every system call you can call from C (`getpid`, `read`, `write`, ...) is
declared in `user/user.h` and implemented by a tiny piece of generated
assembly in `user/usys.S`. That file is *generated* — not hand-written — by
the Perl script `user/usys.pl`. Open `user/usys.S` after building xv6 and
you'll see entries like this:

```asm
.global getpid
getpid:
 li a7, SYS_getpid
 ecall
 ret
```

Two things happen before the trap: the syscall **number** is loaded into
register `a7`, and any **arguments** the function was called with are
already sitting in `a0`–`a5` (this is just the normal RISC-V calling
convention — nothing special yet). Then `ecall` executes.

### Step 2 — The hardware crossing

`ecall` is a single instruction, but it does several things atomically:

1. Raises the privilege level from U-mode to S-mode.
2. Saves the current program counter into `sepc`.
3. Sets `scause` to the value meaning "environment call from U-mode" (`8`).
4. Jumps to the address held in `stvec`.

`stvec` always points at the same place for every process: a tiny piece of
assembly called the **trampoline**, in `kernel/trampoline.S`. This file is
mapped at the *identical* virtual address in both the kernel's page table
and every single user process's page table — the one shared landmark
between two otherwise completely different address spaces. That's
deliberate: when `ecall` fires, the page table is still the *user* page
table for a few more instructions, so the very next code that runs has to
live at an address that means the same thing under both page tables.

### Step 3 — `uservec`: saving the world

The trampoline's `uservec` label does the actual save. It can't use the
stack — it doesn't know if the user stack pointer is even valid — so it
saves every general-purpose register into a fixed, pre-allocated structure
called the **trapframe** (`struct trapframe`, declared in `kernel/proc.h`,
one per process, mapped at a fixed virtual address called `TRAPFRAME`).
Once every register is safely in the trapframe, `uservec` switches `satp`
(the register that selects the page table) from the user page table to the
kernel page table, and jumps into C code: `usertrap()` in `kernel/trap.c`.

### Step 4 — `usertrap()`: deciding what kind of trap this is

`usertrap()` is the first C function that runs. It reads `scause`. If the
value is `8`, this was a system call, and three things happen before
anything else:

- `p->trapframe->epc` is advanced **by 4** (one instruction). Without this,
  the process would return *to* the `ecall` instruction and execute it
  again forever — `ecall` itself doesn't auto-advance the PC the way a
  return-from-trap normally would.
- Interrupts are re-enabled (`intr_on()`), since a system call can run for
  a while and shouldn't block timer interrupts from the rest of the system.
- `syscall()` is called — this is the function you'll be reading and
  editing this week.

If `scause` had been something else (a page fault, a timer interrupt), this
same function would route to a different handler. System calls are just one
of several reasons a trap can happen — they're the only one we touch this
week.

### Step 5 — Returning

Once `syscall()` returns, `usertrap()` finishes up and calls
`usertrapret()`, which restores the saved trapframe values, switches `satp`
back to the user page table, and jumps to `userret` in the trampoline, which
finally executes `sret` — the mirror image of `ecall` — dropping back to
U-mode at the (now advanced) saved program counter. The user program
resumes one instruction after `ecall`, with its return value waiting in
`a0`, having no idea any of this just happened.

```
 USER                                    KERNEL
 ──────────────────────────────────────────────────────────────────
  getpid() stub
   a7 = SYS_getpid
   ecall ───────────────────────────►  uservec (trampoline.S)
                                          save registers → trapframe
                                          switch to kernel pagetable
                                          ▼
                                        usertrap() (trap.c)
                                          scause == 8 ?
                                          epc += 4, intr_on()
                                          ▼
                                        syscall() (syscall.c)
                                          read a7, look up table
                                          call sys_getpid()
                                          store result in a0
                                          ▼
                                        usertrapret() → userret
                                          restore registers
                                          switch to user pagetable
  ◄─────────────────────────────────── sret
  resumes after ecall, a0 = pid
```

---

## 3. Syscall Dispatch: From `a7` to a Function Pointer

`syscall()`, in `kernel/syscall.c`, is short — read it in full, it's worth
memorizing the shape of it. The essence:

```c
void
syscall(void)
{
  int num;
  struct proc *p = myproc();

  num = p->trapframe->a7;
  if (num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    p->trapframe->a0 = syscalls[num]();
  } else {
    printf("%d %s: unknown sys call %d\n", p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}
```

`syscalls` is an array of function pointers, indexed by syscall number,
built from two pieces that must always agree:

- `kernel/syscall.h` — `#define`s the numbers (`SYS_fork`, `SYS_getpid`,
  ...).
- `kernel/syscall.c` — `extern` declares each `sys_*` function, then lists
  them in the array using designated initializers: `[SYS_getpid] sys_getpid,`.

If a user program puts a number in `a7` that has no entry, `syscall()`
doesn't crash — it prints a diagnostic and sets the return value to `-1`.
This is worth noticing: a malformed or malicious syscall number is just a
data value to the kernel, not a control-flow hazard. The indexing is
bounds-checked before the function pointer is ever touched.

---

## 4. Passing Arguments and Returning Values Safely

A `sys_*` function (e.g. `sys_sleep`, `kernel/sysproc.c`) takes **no
parameters** in C — `uint64 sys_sleep(void)`. That looks wrong until you
remember the arguments are already sitting in the trapframe's `a0`–`a5`,
exactly where the user-space stub put them before `ecall`. The helper
functions in `syscall.c` read them back out:

```c
void argint(int n, int *ip);          // n-th argument as an int
int  argaddr(int n, uint64 *ip);      // n-th argument as a user virtual address
int  argstr(int n, char *buf, int max); // n-th argument as a NUL-terminated string
```

`argint` is trivial — it just reads `p->trapframe->a0` through `a5`
(via an internal `argraw(n)`) and returns the raw 64-bit value reinterpreted
as an `int`.

`argaddr` looks just as trivial, but it isn't: it gives you back a number,
and that number is a **user virtual address** — meaningless to the kernel
on its own. This is the single most important idea this week:

> **The kernel and the running process do not share a page table.** A
> pointer value that's valid in user space is just an integer to the
> kernel. Dereferencing it directly (`*buf = 5;`) would either read/write
> the wrong physical memory or fault outright, because the kernel's `satp`
> currently points at the *kernel's* page table, not the process's.

To safely move bytes across that boundary, xv6 provides two functions in
`kernel/vm.c`:

```c
int copyin (pagetable_t pagetable, char *dst, uint64 srcva, uint64 len);
int copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len);
```

Both take the *process's* page table explicitly (usually
`myproc()->pagetable`), walk it page by page to translate the user virtual
address to a physical address, check that the page is actually mapped and
that the access fits inside the process's allocated memory size (`p->sz`),
and only then copy. If the user address is bogus — unmapped, beyond the
process's memory, or just garbage — `copyin`/`copyout` return `-1` instead
of crashing the kernel. `argstr` builds on `copyin` (via `copyinstr`) to
safely fetch a string of unknown length, stopping at a NUL byte or `max`,
whichever comes first.

**The rule for this week:** any time a syscall you write needs to read from
or write to memory the *user* program owns, go through `copyin`/`copyout`.
Never dereference a user pointer directly inside kernel code.

---

## 5. Adding a Custom System Call: Step-by-Step

Adding a new system call always touches the same five places. There's no
shortcut — every one of these is required, and missing one produces a
linker error or a kernel that boots but can't find your syscall.

| # | File | What you add |
|---|---|---|
| 1 | `kernel/syscall.h` | A new `#define SYS_yourcall N` — one past the highest existing number |
| 2 | `user/user.h` | A C prototype, so user programs can call it like any other function |
| 3 | `user/usys.pl` | One `entry("yourcall");` line — this regenerates `usys.S` with the stub at build time |
| 4 | `kernel/syscall.c` | An `extern uint64 sys_yourcall(void);` and an entry in the `syscalls[]` array: `[SYS_yourcall] sys_yourcall,` |
| 5 | `kernel/sysproc.c` (or `sysfile.c` if it's about files) | The actual `uint64 sys_yourcall(void) { ... }` implementation |

A fully worked example — a trivial `hello()` syscall that takes no
arguments and returns a fixed value — is provided in
[`code/00_worked_example_hello_syscall.md`](code/00_worked_example_hello_syscall.md).
Read it before starting the problem set; the problems ask you to add
*different* syscalls using this same five-step pattern, with arguments and
real kernel-state involved.

A couple of habits that will save you time:

- After editing kernel files, always `make clean && make qemu`. Stale
  `.o` files from kernel changes are a classic source of "my change isn't
  taking effect" confusion.
- Forgetting step 3 (`usys.pl`) is the single most common mistake. The
  symptom is a **linker error** mentioning your syscall name when building
  the *user* program — the kernel side is fine, but no stub exists for
  user code to call.
- A syscall number reused by accident (forgetting to check the highest
  existing number) silently overwrites an existing syscall's slot in the
  array. Always grep `kernel/syscall.h` for the current highest number
  before picking yours.

---

## 6. References

- [xv6-riscv book](https://pdos.csail.mit.edu/6.828/2025/xv6/book-riscv-rev4.pdf) — Chapter 4 ("Traps") covers everything in Sections 2–4 above in more formal detail.
- [xv6-riscv source](https://github.com/mit-pdos/xv6-riscv) — `kernel/trampoline.S`, `kernel/trap.c`, `kernel/syscall.c`, `kernel/vm.c` are the files this week's theory is built from. Read them alongside this document.
- OSTEP, "Limited Direct Execution" chapter (in `Resources/`) — the general OS theory behind traps, independent of xv6's specific implementation.
- RISC-V Privileged Architecture spec — the formal definition of `ecall`, `sret`, `scause`, `stvec`, `sepc`, if you want the ground truth instead of our summary.
