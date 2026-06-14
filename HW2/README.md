# Operating Systems Assignment - Debugging, System Analysis & Performance Evaluation

**Course:** Operating Systems  
**Instructor:** Dr. Hossein Asadi

## Overview

This assignment explores several advanced operating system concepts including:

- Debugging the XV6 operating system using GDB.
- Implementing a new system call to retrieve process information similar to Linux `top`.
- Comparing the performance of Bare Metal, Virtual Machine, and Docker environments using `sysbench`.
- Analyzing the effects of Fork Bomb attacks and methods for limiting them.
- Profiling and optimizing C/C++ programs using `perf`, FlameGraph, and Tracy.

Each section contains the implementation, experiment results, generated reports, and performance analysis.

## Topics

### 1. XV6 Debugging & System Call Development
- Kernel debugging using GDB.
- Analyzing syscall execution path.
- Inspecting process structures and CPU state.
- Implementing a `top`-like system call in XV6.

### 2. Environment Performance Comparison
- CPU-intensive benchmarks.
- Memory-intensive benchmarks.
- I/O-intensive benchmarks.
- Performance comparison between:
  - Bare Metal Linux
  - Virtual Machine
  - Docker Container

### 3. Fork Bomb Analysis
- Fork Bomb implementation.
- Resource exhaustion analysis.
- Recovery and prevention mechanisms.

### 4. Program Performance Profiling
- Profiling with Linux `perf`.
- Generating FlameGraphs.
- Finding performance bottlenecks.
- Optimizing inefficient code.
- Runtime analysis using Tracy.
