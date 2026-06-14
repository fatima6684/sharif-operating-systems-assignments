# Linux Kernel Compilation and Kernel Module Development

## Description

This section explores Linux kernel development by compiling a custom kernel configuration and implementing a simple loadable kernel module.

---

## Kernel Configuration

The Linux kernel source code was downloaded and configured with debugging capabilities.

Enabled features include:

- Kernel debugging
- Memory debugging tools (such as KASAN)
- Loadable module support

The kernel was compiled and installed using the customized configuration.

---

## Kernel Module Development

A simple kernel module named `mymodule` was implemented.

The module prints messages when:

- It is loaded into the kernel.
- It is removed from the kernel.

The module was tested using:

- `insmod` for loading the module
- `rmmod` for removing the module
- `dmesg` and kernel logs for verification

---

## Files

- `mymodule.c` — Kernel module implementation
- `Makefile` — Build configuration
- Kernel configuration files
- Screenshots of successful module loading and unloading
- Final report containing experiments and observations
