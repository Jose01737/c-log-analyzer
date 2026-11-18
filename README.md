# C Multithreaded Log Analyzer

A small C program that reads a text log and counts INFO, WARNING, and ERROR entries using worker threads.

## Requirements
- GCC or Clang
- POSIX threads (pthreads)

## Build
```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -pthread log_analyzer.c -o log_analyzer
```

## Run
```bash
./log_analyzer sample.log 4
```
The second argument is the number of worker threads (1–16).

## Log format
Each line should contain one of: `INFO`, `WARNING`, or `ERROR`.
Unknown lines are counted separately.

## What I learned
C file I/O, pthreads, synchronization, input validation, and dividing work across threads.
