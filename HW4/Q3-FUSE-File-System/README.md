# User-Space File System with FUSE

## Overview

This project implements a simple file system in user space using the FUSE architecture.

The file system stores its data inside a fixed-size disk image file (`db.filesys`) and manages files using custom metadata structures.

## Implemented Features

### File System Metadata

Implemented a metadata structure containing:

- Magic number validation (`0xDEADBEEF`)
- File system version information
- Storage usage information

### File Management

Designed file structures containing:

- File name
- File type
- Permissions
- File data

Implemented support for:

- `open()`
- `read()`
- `write()`
- `shrink()`
- `close()`
- `rm()`
- `stats_file_get()`
- `stats_fs_get()`

### Storage Management

Implemented a simple free-space management mechanism by tracking the end of the last allocated block inside the storage image.

## Testing

Tested file creation, reading, writing, resizing, deletion, and filesystem statistics to verify correctness and persistence.
