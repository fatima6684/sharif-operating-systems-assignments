# Homework 03 - Operating Systems

This homework explores process management in xv6, Linux container isolation, Linux kernel system call implementation, and system call performance analysis.

## Topics Covered

- Process tree visualization in xv6
- Process Control Block (PCB) structures
- Parent-child process relationships
- Linux PID namespaces
- Container process isolation
- Custom Linux system calls
- Linux kernel modification
- System call latency measurement
- User-Kernel transition analysis

## Questions

### Q1 - Process Tree Visualization in xv6

Extended the xv6 process monitoring utilities by implementing a new system call to retrieve process information and display the process hierarchy in a tree format.

### Q2 - PID Namespace Isolation in Containers

Implemented process isolation using Linux PID namespaces and modified a custom container runtime to execute processes inside independent PID namespaces.

### Q3 - Linux System Call Development and Performance Analysis

Developed custom Linux system calls, modified the Process Control Block (PCB), and measured the execution overhead of different system calls.
