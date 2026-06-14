
---

# `Question2-Docker-OverlayFS/README.md`

```markdown
# Question 2 - Docker OverlayFS Container Implementation

## Introduction

The goal of this exercise was to modify the previous container implementation to use a layered filesystem similar to Docker's OverlayFS mechanism.

Instead of copying an entire image into a container directory, the filesystem is constructed using multiple read-only image layers and a separate writable layer.

---

## Implementation Details

Several modifications were applied to the container runtime:

- Added the `--image-base` command line option.
- Modified `dir_container_setup()` in `setup.c`.
- Read the lower layers of the Docker image from OverlayFS metadata.
- Created an OverlayFS mount containing:
  - Lower directories (image layers)
  - Upper directory (container changes)
  - Work directory
  - Merged directory

The `merged` directory was used as the root filesystem of the container.

---

## Advantages Over Previous Implementation

The previous implementation copied the whole filesystem for every container. This approach had several disadvantages:

- High disk usage
- Slow container creation
- Duplicate copies of the same image files

Using OverlayFS solves these problems because multiple containers can share the same read-only layers, while each container stores only its own modifications.

---

## Docker Commit

The `docker commit` command creates a new image from the current state of a container.

A similar implementation can be achieved by storing the container's writable layer as a new image layer and linking it with the previous read-only layers.

---

## Image vs Container

### Image
- Read-only template.
- Contains application files and dependencies.
- Shared between multiple containers.

### Container
- Running instance of an image.
- Has its own writable layer.
- Contains runtime state and processes.

---

## Testing

The implementation was tested by running a Python Fibonacci program inside the created container:

```bash
./zocker run --image-base docker.arvancloud.ir/python:3.11 python fibonacci.py
