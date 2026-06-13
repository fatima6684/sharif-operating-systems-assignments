# Memory Allocation Functions in xv6

## Overview

This project extends the xv6 user-space memory allocator by implementing the standard C library functions `realloc()` and `calloc()`.

## Implemented Features

### realloc()

Implemented dynamic memory resizing with support for:

- Allocating new memory when the input pointer is `NULL`
- Freeing memory when the requested size is zero
- Allocating a new memory block
- Copying existing data to the new location
- Releasing the old memory block

### calloc()

Implemented zero-initialized memory allocation by:

- Allocating memory for an array of elements
- Initializing all allocated bytes to zero

## Testing

Developed user-level test programs to verify:

- Correct memory allocation
- Data preservation after `realloc()`
- Proper zero initialization in `calloc()`
- Edge cases such as `NULL` pointers and zero-sized allocations
