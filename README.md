# Linux Kernel Projects

Hands-on Linux kernel programming projects demonstrating
progressively deeper kernel concepts.

## Projects

### 01 - Hello World Kernel Module
Basic kernel module lifecycle - init, exit, printk.
Foundation of every Linux device driver.

### 02 - /proc Filesystem Driver
Custom /proc entry exposing system metrics from kernel.
Connects to userspace Linux system monitor project.

### 03 - Character Device Driver
Virtual hardware accelerator with ioctl interface.
Same pattern used in GPU and HPC accelerator drivers.

### 04 - Slab Memory Allocator
User space implementation of Linux kernel slab allocator.
Demonstrates kmalloc/kfree concepts and memory management.

## Environment
- Ubuntu Linux (Lima VM on macOS)
- Kernel: 6.17.0-14-generic
- Build: Linux Kbuild system + gcc

## Reading
- Linux Kernel Development - Robert Love
- The Linux Programming Interface - Kerrisk
- Linux Device Drivers - Corbet, Rubini, Kroah-Hartman
  (free at lwn.net/Kernel/LDD3)