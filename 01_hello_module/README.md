# Hello World Linux Kernel Module

Demonstrates the fundamental structure of a Linux kernel loadable module - the foundation of all Linux device drivers.

## What This Demonstrates

**Kernel module lifecycle:**
- `module_init` - called by `insmod`, sets up module
- `module_exit` - called by `rmmod`, cleans up
- `printk` - kernel logging to ring buffer (dmesg)
- `__init`/`__exit` - memory optimization macros

**Why kernel modules matter:**
Every Linux device driver - GPU drivers, NIC drivers, storage controllers - follows this exact pattern.
The init function sets up hardware, registers interfaces, allocates resources. The exit function reverses everything.

**Kernel vs userspace:**
- No `printf` - use `printk` with log levels
- No `malloc` - use `kmalloc`/`vmalloc`
- No standard library - only kernel APIs
- Bugs don't crash the process - they crash the kernel

## Build and Run

```bash
# Install kernel headers
sudo apt install linux-headers-$(uname -r) build-essential

# Build
make

# Load module
sudo insmod hello_module.ko

# Verify loaded
lsmod | grep hello
dmesg | tail

# Unload module
sudo rmmod hello_module
```

## Files
hello_module.c  - kernel module source
Makefile        - kernel build system integration

## What's Next

This hello world module is extended in:
- `02_proc_monitor` - creates /proc filesystem entry
- `04_char_device`  - implements hardware accelerator interface

## Environment

- Ubuntu Linux on Lima VM
- Kernel: 6.17.0-14-generic
- Build system: Linux kernel Kbuild
