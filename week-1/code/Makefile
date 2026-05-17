# Makefile for Week 1 code examples — Multithread Mayhem
#
# Usage:
#   make            # build everything
#   make clean      # remove built binaries
#   make hello      # build only the hello thread example
#   make race       # build only the race counter example
#   make sum        # build only the parallel sum example
#   make bank       # build only the bank chaos example
#   make run-all    # build and run every example in order
#
# Requires gcc and pthreads. On Debian/Ubuntu these are in
# the build-essential package. On macOS, install Xcode command
# line tools.

CC      = gcc
CFLAGS  = -Wall -Wextra -pthread -O0 -g
TARGETS = hello_thread race_counter parallel_sum bank_chaos

.PHONY: all clean run-all hello race sum bank

all: $(TARGETS)

hello: hello_thread
race:  race_counter
sum:   parallel_sum
bank:  bank_chaos

hello_thread: 01_hello_thread.c
	$(CC) $(CFLAGS) $< -o $@

race_counter: 02_race_counter.c
	$(CC) $(CFLAGS) $< -o $@

parallel_sum: 03_parallel_sum.c
	$(CC) $(CFLAGS) $< -o $@

bank_chaos: 04_bank_chaos.c
	$(CC) $(CFLAGS) $< -o $@

run-all: all
	@echo "=== 01 hello_thread ==="
	./hello_thread
	@echo
	@echo "=== 02 race_counter ==="
	./race_counter
	@echo
	@echo "=== 03 parallel_sum ==="
	./parallel_sum
	@echo
	@echo "=== 04 bank_chaos ==="
	./bank_chaos

clean:
	rm -f $(TARGETS)
