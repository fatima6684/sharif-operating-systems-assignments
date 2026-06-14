# User-Level Threads and Context Switching

## Overview

This project implements a cooperative user-level threading mechanism inspired by operating system thread scheduling.

Each thread maintains its own execution context, including CPU register states and a private stack. The scheduler saves the current context and restores the next runnable thread using context switching.

## Implemented Features

- Designed a `context` structure for saving CPU registers
- Implemented a thread control structure containing stack, state, and execution context
- Developed `init_thread()` to initialize the main thread
- Implemented `create_thread()` for creating new runnable threads
- Designed a cooperative scheduler in `schedule_thread()`
- Implemented `yield_thread()` for voluntarily giving up the CPU

## Testing

Implemented a multi-threaded test program containing three threads that repeatedly yield execution and demonstrate correct context switching behavior.

The output confirms proper thread scheduling, execution order, and thread termination.
