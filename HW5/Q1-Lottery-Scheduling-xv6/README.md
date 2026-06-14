# Lottery Scheduling in xv6

## Overview

This project extends the xv6 operating system by implementing a lottery-based CPU scheduler.

Each runnable process owns a number of lottery tickets. During each scheduling cycle, a random ticket is selected and the corresponding process receives CPU time.

## Implemented Features

- Added a `ticket` field to the process control block (`struct proc`)
- Initialized default tickets for newly created processes
- Modified `fork()` to inherit tickets from the parent process
- Redesigned the `scheduler()` function to select processes based on lottery probabilities
- Added a custom system call to modify the ticket count of a process
- Implemented validation for invalid PIDs and ticket values

## Testing

Developed a user-space testing program that:

- Creates multiple child processes using `fork()`
- Assigns different ticket values using the custom syscall
- Runs CPU-intensive loops
- Measures CPU allocation fairness according to ticket ratios

## Environment

The scheduler was tested in single CPU mode:

```bash
CPUS=1 make qemu
