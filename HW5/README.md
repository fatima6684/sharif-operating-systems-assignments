# Homework 06 - Operating Systems

This homework focuses on CPU scheduling algorithms, container isolation mechanisms, and file system space management.

## Topics Covered

- Lottery-based CPU scheduling in xv6
- Process management and system calls
- Kernel modifications and process metadata
- Container filesystem isolation using `chroot`
- Linux filesystem hierarchy and shared libraries
- Custom filesystem CLI development
- Free space management using linked lists

## Questions

### Q1 - Lottery Scheduling in xv6

Implemented a probabilistic CPU scheduling algorithm based on lottery tickets. Processes receive a number of tickets that determine their probability of being selected by the scheduler.

### Q2 - Container Root Filesystem Isolation

Investigated how Docker isolates container filesystems and implemented root filesystem isolation in a custom container runtime using `chroot`.

### Q3 - File System Free Space Management

Redesigned the filesystem free space allocator using a linked-list-based structure and developed a command-line interface to interact with the filesystem.
