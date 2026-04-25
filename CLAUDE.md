# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

User-level threads library (`uthreads`) for Hebrew University OS course (67808), Exercise 1. Due: 7/5/2026.

Implements a preemptive Round-Robin thread scheduler entirely in user space using `SIGVTALRM` + `setitimer` for quantum expiry and `sigsetjmp`/`siglongjmp` for context switching.

## Build

No Makefile. Compile flags: `-Wall -Werror -Wpedantic --std=c++17`. Do **not** use `-O3`.

```bash
# Compile a test
g++ -Wall -Werror -Wpedantic --std=c++17 -I. uthreads.cpp tests/test_1_kick_tires.cpp -o program
./program
```

## Tests

```bash
# Run all tests
/usr/bin/python3 tests/run_tests.py

# Run a single test by name
/usr/bin/python3 tests/run_tests.py test_1_kick_tires
```

Each test is a `.cpp` + `.txt` pair under `tests/`; the runner compiles, runs, and diffs stdout against the `.txt` expected output.

**School solution** (authoritative reference for any ambiguity):
```bash
~os/ex1/run_school_solution tests/test_1_kick_tires.cpp
# accepts multiple files or a directory
```

## Architecture

### Public API
- **`uthreads.h`** — the only public interface; do not modify it
- **`uthreads.cpp`** — full implementation goes here (currently stubs)

### Key constants (`uthreads.h`)
- `MAX_THREAD_NUM = 100` — max concurrent threads including main
- `STACK_SIZE = 4096` — per-thread stack in bytes

### Thread model
- Thread 0 (main) reuses its existing stack/registers; no manual setup needed
- All other threads get a heap-allocated stack of `STACK_SIZE`
- States: `RUNNING` → `READY` → `BLOCKED` (see state diagram in PDF)
- READY threads are managed as a **queue** (FIFO); new/resumed threads go to the tail
- A thread that is both sleeping and blocked must wait for **both** conditions to clear before becoming READY

### Scheduling
- Virtual timer (`ITIMER_VIRTUAL`) fires `SIGVTALRM` every `quantum_usecs` microseconds
- Signal handler performs a context switch: save current thread with `sigsetjmp`, restore next with `siglongjmp`
- `translate_address` (from `demo_jmp.c`) **must** be used when setting up stack pointers — see `demos/demo_jmp.c`
- When a thread blocks or sleeps, its remaining quantum is forfeited (scheduler proceeds as if quantum expired)
- Sleep counter is decremented each quantum; thread re-enters READY when counter hits 0 (unless also blocked)

### Signal safety
- Block `SIGVTALRM` with `sigprocmask` around any scheduler data structure access to prevent races
- Always check return values of system calls; on failure print `"system error: <text>\n"` to stderr and `exit(1)`
- On library misuse print `"thread library error: <text>\n"` to stderr and return `-1`; nothing else goes to stderr/stdout

### Reference demos (read-only, do not modify)
- `demos/demo_itimer.c` — `setitimer` + `SIGVTALRM` usage pattern
- `demos/demo_jmp.c` — `sigsetjmp`/`siglongjmp` + `translate_address` pattern
- `demos/demo_singInt_handler.c` — `sigaction` setup pattern

### Games (optional, test your implementation)
```bash
g++ -Wall --std=c++17 -I. uthreads.cpp games/ants.cpp -o ants && ./ants
g++ -Wall --std=c++17 -I. uthreads.cpp games/thread_tron.cpp -o tron && ./tron
```

## Submission
```
ex2.tar: README (per course guidelines) + all source files except uthreads.h
```
