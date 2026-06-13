# OverlayFS and Docker Layering

## Overview

This project explores Linux OverlayFS and its usage in Docker image layering.

The experiments investigate how multiple filesystem layers interact and how Copy-On-Write mechanisms reduce unnecessary data duplication.

## Experiments

### Single Overlay Layer

Created a filesystem using:

- Lower directory
- Upper writable directory
- Work directory
- Merged mount point

Investigated:

- Inode behavior between lower and merged layers
- File creation location
- The role of the work directory
- Copy-up behavior after modifying files

### Multiple Lower Layers

Created an OverlayFS with multiple lower layers and analyzed:

- Layer priority
- File visibility
- Inode changes across layers

### Docker Layer Build Simulation

Implemented a dynamic Bash script that simulates a simplified Docker build process.

Each `RUN` instruction creates a new writable layer, which is later used as a lower layer for the next stage.

## Concepts

- OverlayFS
- Copy-On-Write
- Docker image layers
- Mount namespaces
- Linux filesystem internals
