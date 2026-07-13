# Makefile for Week 7 code examples — Multithread Mayhem
#
# NOTE: This program is meant to run INSIDE xv6, not on your host OS.
# To use it, copy the .c file into xv6-riscv/user/ and add it to the
# UPROGS list in the xv6 Makefile, then:
#
#   make clean && make && make qemu
#
# This Makefile is provided for reference only — the xv6 Makefile is
# what actually builds this program into a RISC-V binary.
#
# 00_worked_example_am_i_a_thread.md is not a program — it's a walkthrough
# of a kernel change. There is nothing to build for it directly; follow
# its steps in your own xv6-riscv checkout.

CC      = gcc
CFLAGS  = -Wall -Wextra -O0 -g
TARGETS = addrspace_probe

.PHONY: all clean

all: $(TARGETS)

addrspace_probe: 01_addrspace_probe.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGETS)
