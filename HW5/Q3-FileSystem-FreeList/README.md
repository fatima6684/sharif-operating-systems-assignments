# Linked-List-Based Free Space Management

## Overview

This project improves the custom file system by redesigning the free space management mechanism.

The original free list was replaced with a linked-list data structure where each node stores the start and end addresses of a free block range.

## Implemented Features

- Developed a command-line interface for filesystem operations
- Redesigned the free list representation using linked lists
- Implemented `alloc(size)` for allocating contiguous free blocks
- Implemented `free(start, size)` for releasing blocks
- Merged adjacent free ranges to prevent fragmentation
- Updated previous filesystem operations to use the new allocator
- Added a `viz` command for visualizing free space layout

## Testing

Tested filesystem commands and verified:

- Correct block allocation
- Correct block deallocation
- Proper merging of neighboring free regions
- Accurate visualization of the free space map
