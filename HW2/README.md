# OS Assignment 02 - XV6 Debugging, System Calls and Performance Analysis

Course: Operating Systems  
Instructor: Dr. Hossein Asadi  
University: Sharif University of Technology

## Overview

This assignment focused on several fundamental operating system concepts:

- Debugging the XV6 kernel using GDB
- Understanding system calls and kernel-user interactions
- Implementing a custom `top`-like system call in XV6
- Benchmarking different execution environments
- Studying the impact of Fork Bomb attacks
- Performance profiling using Linux tools such as `perf`, FlameGraph and Tracy


## Components

### 1. XV6 GDB Debugging

Analyzing the execution flow of system calls in XV6 using GDB, inspecting kernel data structures, stack frames, and processor registers.

### 2. Custom Top System Call

Implemented a new system call that provides information about running processes, including:

- Process ID (PID)
- Process name
- Allocated memory size

A user-space `top` command was also developed to display the collected information.

### 3. Performance Analysis

Developed a computationally expensive C++ program and analyzed its execution using:

- Linux `perf`
- FlameGraph
- Tracy profiler

Performance bottlenecks were identified and the code was optimized based on the profiling results.


### 4. Fork Bomb Analysis

Implemented and analyzed a Fork Bomb attack in a controlled environment, studying:

- Process explosion behavior
- Resource exhaustion
- Methods for limiting and recovering from the attack


## Technologies

- C
- C++
- XV6 Operating System
- GDB
- Linux System Calls
- perf
- FlameGraph
- Tracy
