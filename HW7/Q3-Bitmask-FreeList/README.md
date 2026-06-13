# Bitmap-Based Free Space Management

## Overview

This project improves the file system block allocation mechanism by replacing the linked-list based free space tracker with a bitmap-based free list.

A bitmap provides a compact and efficient representation of block allocation status.

## Implemented Features

- Designed a bitmap structure for tracking free and allocated blocks
- Reserved metadata and bitmap blocks in the filesystem layout
- Modified allocation and deallocation algorithms
- Implemented stress testing with large numbers of file operations

## Performance Analysis

The performance of the bitmap-based allocator was compared with the original linked-list implementation.

Profiling tools such as `perf` and tracing utilities were used to identify performance bottlenecks and evaluate the improvement.

## Stress Test

The filesystem was tested using randomized operations including:

- Creating files
- Deleting files
- Reading data
- Writing data
- Resizing files
