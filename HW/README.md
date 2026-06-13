# Homework 07 - Operating Systems

This homework focuses on advanced operating system concepts, including memory management, layered file systems, and filesystem performance optimization.

## Topics Covered

- Copy-On-Write (COW) fork implementation in xv6
- Virtual memory and page table management
- Page fault handling
- Physical page reference counting
- Linux OverlayFS and Docker image layers
- Copy-on-write behavior in layered file systems
- File system free space management using bitmaps
- Performance analysis and benchmarking

## Questions

### Q1 - Copy-On-Write Fork in xv6

Implemented a Copy-On-Write version of `fork()` in the xv6 operating system to reduce unnecessary memory copying. The implementation includes page sharing, page fault handling, and reference counting.

### Q2 - OverlayFS and Docker Layer Simulation

Explored the behavior of Linux OverlayFS, including lower and upper layers, inode behavior, copy-on-write semantics, and a simplified Docker image layering mechanism using Bash scripts.

### Q3 - Bitmap-Based Free Space Management

Improved the file system free space management algorithm by replacing the linked-list based free list with a bitmap-based structure and analyzed its performance using profiling tools.
