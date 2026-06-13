# File System Users, Groups, and Permissions

## Overview

This project extends the custom file system by adding Unix-like user management and access control mechanisms.

## Implemented Features

### User and Group Management

Implemented support for:

- Root user initialization
- Creating and deleting users
- Creating and deleting groups
- Adding users to groups

Supported commands:

- `useradd`
- `userdel`
- `groupadd`
- `groupdel`
- `usermod -aG`

## File Ownership and Permissions

Added metadata to each file for:

- Owner user
- Owner group
- Access permissions for owner, group, and others

Implemented commands:

- `chmod`
- `chown`
- `chgrp`
- `getfacl`

## Access Control

Modified existing filesystem operations to enforce permission checks based on the requesting user's privileges and the file's access mode.
