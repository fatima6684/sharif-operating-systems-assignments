# Docker Container Runtime and Filesystem Exploration

## Overview

This project explores the internal behavior of Docker containers and implements support for running containers from a custom filesystem directory.

## Experiments

### Docker Image and Container Execution

- Pulled and executed an Alpine Linux image
- Investigated the behavior of the `--rm` flag
- Exported the container filesystem using `docker export`

### Filesystem Analysis

Compared the exported filesystem with a running container and analyzed differences in virtual filesystem components such as `/proc`.

### Custom Container Base Directory

Modified the `container_run` function to allow creating containers from a user-defined base directory.

## Discussion

This implementation copies the entire container filesystem, which is not the approach used by Docker in production.

Real Docker uses layered filesystems such as OverlayFS to allow multiple containers to share read-only image layers while maintaining isolated writable layers for each container.
