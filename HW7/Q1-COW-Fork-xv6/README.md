# Copy-On-Write Fork in xv6

## Overview

This project implements Copy-On-Write (COW) support for the `fork()` system call in the xv6 operating system.

Instead of copying all user memory pages during `fork`, parent and child processes initially share physical pages. A private copy of a page is created only when one of the processes attempts to modify it.

## Implemented Features

- Modified `uvmcopy()` to share physical pages between parent and child
- Marked shared pages as read-only
- Added a COW flag using RSW bits in the RISC-V page table entries
- Implemented page fault handling in `usertrap()`
- Added physical page reference counting in `kalloc.c`
- Updated `copyout()` to correctly handle COW pages

## Testing

The implementation was validated using xv6 test programs:

- `cowtest`
- `usertests`

Both test suites successfully passed.
