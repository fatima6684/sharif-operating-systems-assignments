# Question 1 - Reader Writer Spinlock in XV6

## Objective

The objective of this exercise is to replace the regular spinlock used for accessing the global `ticks` variable with a Reader-Writer Spinlock (RW Spinlock).

Unlike a normal spinlock, RW Spinlock allows multiple readers to access a shared resource simultaneously while still ensuring exclusive access for writers.

## Implementation

The `rwspinlock` structure was modified in `kernel/spinlock.h` to store the state of readers and writers.

The following functions were implemented:

- `read_acquire_inner()`
- `write_acquire_inner()`
- `inner_release_read()`
- `inner_release_write()`

The implementation follows the writer-priority policy:
- Multiple readers can hold the lock at the same time.
- Only one writer can hold the lock.
- If a writer is waiting, new readers are blocked until the writer finishes execution.

Atomic operations were used to avoid race conditions between CPUs.

## Testing

The implementation was tested using the provided `rwlktest` program.

Expected output:
