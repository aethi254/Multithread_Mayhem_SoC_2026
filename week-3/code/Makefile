# Makefile for Week 3 code examples — Multithread Mayhem

CC     = gcc
CFLAGS = -Wall -Wextra -pthread -std=c11 -O0 -g
TARGETS = thread_lifecycle return_values semaphores readers_writers

.PHONY: all clean run-all

all: $(TARGETS)

thread_lifecycle: 01_thread_lifecycle.c
	$(CC) $(CFLAGS) $< -o $@

return_values: 02_return_values.c
	$(CC) $(CFLAGS) $< -o $@

semaphores: 03_semaphores.c
	$(CC) $(CFLAGS) $< -o $@

readers_writers: 04_readers_writers.c
	$(CC) $(CFLAGS) $< -o $@

run-all: all
	@echo "=== 01 thread_lifecycle ==="
	./thread_lifecycle
	@echo
	@echo "=== 02 return_values ==="
	./return_values
	@echo
	@echo "=== 03 semaphores ==="
	./semaphores
	@echo
	@echo "=== 04 readers_writers ==="
	./readers_writers

clean:
	rm -f $(TARGETS)
