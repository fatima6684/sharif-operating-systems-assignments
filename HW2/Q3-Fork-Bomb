# Fork Bomb Analysis and Program Performance Profiling

## Description

This section explores two important aspects of operating systems: system reliability under process exhaustion attacks and software performance analysis.

The first part investigates the behavior of a Fork Bomb attack, its impact on system resources, and different approaches to prevent or control such situations.

The second part focuses on profiling and optimizing a computationally expensive C/C++ application using Linux performance analysis tools including `perf`, FlameGraph, and Tracy.

---

## Part 1: Fork Bomb Analysis

### Overview

A Fork Bomb is a denial-of-service technique where a process repeatedly creates child processes, causing exponential growth in the number of processes. This can exhaust available system resources such as CPU time, process table entries, and memory.

### Implementations and Experiments

- Implemented a simple Fork Bomb program.
- Executed the program inside a controlled virtual machine environment.
- Observed the effects of rapid process creation on system performance.
- Analyzed why stopping the attack using `Ctrl+C` may not immediately recover the system.
- Investigated methods for limiting and preventing Fork Bomb attacks.

### Prevention Techniques

The following mechanisms were studied to control Fork Bomb attacks:

- Limiting the maximum number of processes per user using `ulimit`.
- Applying process limits through system configuration.
- Monitoring and terminating malicious processes.
- Running untrusted programs inside isolated environments such as containers or virtual machines.

---

## Part 2: Performance Profiling and Optimization

### Overview

Performance profiling helps identify bottlenecks in software by analyzing CPU usage, function execution time, and resource consumption.

A computationally intensive C/C++ program containing inefficient implementations was developed and analyzed.

### Tools

#### perf

Linux `perf` was used to collect low-level performance statistics including CPU cycles, function execution time, and hardware events.

#### FlameGraph

FlameGraph was generated using `perf` outputs to visualize function call stacks and identify performance bottlenecks.

#### Tracy

Tracy was used as a real-time profiler to inspect application behavior and measure execution characteristics.

---

## Optimization Process

The following steps were performed:

1. Developed an initial inefficient implementation.
2. Collected performance data using `perf`.
3. Generated FlameGraphs to detect expensive functions.
4. Optimized identified bottlenecks.
5. Compared performance before and after optimization.
6. Analyzed runtime behavior using Tracy.

---

## Files

- `fork_bomb/` — Fork Bomb implementation and experiment results.
- `profiling_program/` — Original and optimized C/C++ source code.
- `perf_results/` — Performance profiling outputs.
- `flamegraphs/` — Generated FlameGraph visualizations.
- `tracy_results/` — Runtime profiling reports.
- `report.pdf` — Experiment explanations, observations, and conclusions.

---

## Notes

All Fork Bomb experiments were performed inside a virtual machine to prevent damage or instability on the host operating system.
