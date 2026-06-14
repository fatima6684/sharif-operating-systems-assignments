
---

# `Question3-Concurrent-FileSystem/README.md`

```markdown
# Question 3 - Concurrent File System Synchronization

## Introduction

The previous filesystem implementation was designed for a single user and a single execution flow. In this exercise, synchronization mechanisms were added to support concurrent access by multiple users or threads.

---

## Concurrency Problems

Without synchronization, several issues may happen:

- Race conditions during file creation and deletion.
- Corruption of filesystem metadata.
- Incorrect updates of free space information.
- Simultaneous writes causing inconsistent file contents.
- Conflicts during memory or block allocation.

---

## Testing Concurrent Access

Different concurrent scenarios were tested:

- Multiple users creating files at the same time.
- Multiple users writing to the same file.
- Simultaneous allocation and freeing of storage blocks.

Without locks, these situations could lead to inconsistent states or corrupted filesystem data.

---

## Synchronization Mechanism

Locks were added to protect critical sections:

- Metadata lock to protect global filesystem information.
- File-level locks to protect individual file contents.
- Free list lock to protect allocation and deallocation operations.

These locks allow safe concurrent execution of independent operations.

---

## Weaknesses and Limitations

Although locking solves concurrency issues, it introduces several challenges:

### Reduced Performance

Large critical sections can decrease parallel execution because threads spend time waiting for locks.

### Deadlock

Incorrect lock ordering may cause multiple threads to wait for each other indefinitely.

### Starvation

Some threads may experience long waiting times if other threads continuously acquire locks.

These scenarios were demonstrated by modifying execution timing and creating specific scheduling conditions.

---

## Conclusion

By introducing synchronization primitives, the filesystem became safe for multi-user and multi-threaded environments. However, careful lock design is required to balance correctness and performance.
