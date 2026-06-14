# Linux Container Isolation using Namespaces

## Overview

This project explores how Linux containers achieve isolation using namespaces.

A custom container runtime was extended to support multiple namespace types and provide stronger separation from the host operating system.

## Implemented Features

### Process and Mount Isolation

- Investigated why `ps` initially displayed host processes
- Fixed `/proc` mounting to correctly reflect container processes
- Handled mount propagation issues using appropriate mount flags

### Additional Namespace Support

Added isolated namespaces for:

- PID namespace
- Mount namespace
- UTS namespace
- User namespace
- Time namespace

### Container Identity

Implemented `sethostname()` so each container exposes its own hostname instead of the host machine hostname.

## Testing

Verified isolation using Linux utilities such as:

- `ps`
- `hostname`
- `ls`
- `pwd`
