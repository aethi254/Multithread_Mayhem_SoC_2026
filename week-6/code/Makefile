# Makefile for Week 6 code examples — Multithread Mayhem
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
# 02_worked_example_sched_print.md is a walkthrough of kernel changes —
# there is nothing to build for it directly. Follow its steps in your
# own xv6-riscv checkout.

CC      = gcc
CFLAGS  = -Wall -Wextra -O0 -g
TARGETS = sched_trace

.PHONY: all clean

all: $(TARGETS)

sched_trace: 01_sched_trace.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGETS)
