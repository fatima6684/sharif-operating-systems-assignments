# PID Namespace Isolation in Containers

## Overview

This project investigates how Linux containers isolate process identifiers using PID namespaces.

A custom container runtime was modified to execute processes inside a separate PID namespace, making the first process inside the container appear as PID 1.

## Implemented Features

- Investigated `/proc/self/ns/pid` and Linux namespace identifiers
- Compared `fork() + unshare()` and `clone()` approaches for namespace creation
- Implemented PID namespace creation using `fork()` and `unshare()`
- Configured the project to run without `sudo` using Linux capabilities (`setcap`)
- Studied namespace switching using `setns()` (optional)

## Testing

Verified PID namespace isolation by:

- Comparing namespace identifiers before and after isolation
- Confirming that the container process received PID 1 inside the namespace
- Executing commands inside the isolated environment
