# Linux System Call Development and Performance Analysis

## Overview

This project explores the architecture of Linux system calls by implementing custom kernel-level functionality and analyzing system call execution overhead.

## Part 1: Custom System Calls

Implemented several custom system calls:

- A simple `hello world` system call that prints a message from the kernel
- A modified version that communicates with user-space output
- System calls for reading and writing a custom field added to the Process Control Block (PCB)

## Kernel Modifications

- Studied the Linux `task_struct` implementation as the process control block
- Added a new field to the process descriptor
- Updated the system call table and kernel source code

## Part 2: System Call Timing Analysis

Implemented a high-resolution timing system call and measured:

- Time before entering kernel space
- Time immediately after entering the system call handler
- Time before returning to user space
- Time after returning to the user program

## Benchmarking

Compared the overhead of different system calls:

- `read()`
- `write()`
- `getpid()`
- Custom system calls

The collected results were used to analyze user-to-kernel and kernel-to-user transition costs.
