# Process Tree Visualization in xv6

## Overview

This project extends xv6 process inspection tools by implementing a system call that provides process information to user-space programs.

The collected process information is used to reconstruct and display the parent-child relationships between processes in a tree structure.

## Implemented Features

- Added `next_process()` system call for process enumeration
- Designed `process_data` structure containing process metadata
- Retrieved process ID, parent ID, heap size, process state, and name
- Extended the `top` command with tree visualization options:
  - `top -t`
  - `top --tree`
- Displayed process states including:
  - Running
  - Runnable
  - Sleeping
  - Zombie

## Testing

Created user-space programs that:

- Generated multiple processes using `fork()`
- Executed child programs using `exec()`
- Collected process information through the new system call
- Reconstructed and printed the process hierarchy

The implementation was validated by comparing the generated tree with the expected process relationships.
