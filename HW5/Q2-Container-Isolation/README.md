
---

# `Q2-Container-Isolation/README.md`

```markdown
# Container Root Filesystem Isolation with chroot

## Overview

This project explores how container runtimes isolate their filesystem from the host operating system.

A custom container runtime was modified to use the `chroot()` system call and create an isolated root directory for each container.

## Implemented Features

- Investigated Docker container root filesystem behavior
- Compared the container root with the host filesystem
- Added `chroot()` support to isolate the container environment
- Created required filesystem directories after changing the root
- Modified binary and library setup functions to correctly provide executable dependencies

## Analysis

The initial implementation failed because essential directories and shared libraries were unavailable inside the new root filesystem.

Using tools such as `ldd`, required shared libraries were identified and copied into the container environment.

## Verification

Container isolation was verified using commands:

- `pwd`
- `ls`
- `cd`

The results showed that the container had a separate filesystem hierarchy from the host.
