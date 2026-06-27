# Makefile for Week 5 code examples — Multithread Mayhem
#
# NOTE: These programs are meant to run INSIDE xv6, not on your host OS.
# To use them, copy the .c files into xv6-riscv/user/ and add them to
# the UPROGS list in the xv6 Makefile, then:
#
#   make clean && make && make qemu
#
# This Makefile is provided for reference only — the xv6 Makefile is
# what actually builds these programs into RISC-V binaries.
#
# 00_worked_example_hello_syscall.md is not a program — it's a walkthrough
# of a kernel change. There is nothing to build for it directly; follow
# its steps in your own xv6-riscv checkout.

CC      = gcc
CFLAGS  = -Wall -Wextra -O0 -g
TARGETS = arg_passing_demo self_timing_demo

.PHONY: all clean

all: $(TARGETS)

arg_passing_demo: 01_arg_passing_demo.c
	$(CC) $(CFLAGS) $< -o $@

self_timing_demo: 02_self_timing_demo.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGETS)
