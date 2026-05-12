# Virtual Hardware Accelerator - Character Device Driver

A Linux kernel character device driver that simulates
a hardware accelerator interface using the standard
kernel driver patterns used in real GPU and
accelerator drivers.

## What This Demonstrates

**Character device driver pattern:**
The foundation of every hardware driver in Linux.
GPUs, NICs, storage controllers - all expose
themselves to userspace through this exact interface.

**Kernel APIs used:**
- `alloc_chrdev_region` - dynamic major number allocation
- `cdev_init/cdev_add` - character device registration
- `class_create/device_create` - /dev node creation
- `copy_from_user/copy_to_user` - safe kernel-userspace data transfer
- `kmalloc/kfree` - kernel memory allocation
- `ioctl` - hardware control commands

**ioctl interface:**
Real hardware accelerators (NVIDIA GPUs, Intel QAT,
AWS Nitro) are controlled via ioctl commands.
This driver implements four commands:
- ACCEL_PROCESS    - trigger data processing
- ACCEL_RESET      - reset device state
- ACCEL_GET_STATUS - query data ready status
- ACCEL_GET_COUNT  - query operation count

**Error handling:**
Proper cleanup on every failure path using
goto-based error handling - the standard Linux
kernel pattern.

## Build and Run

```bash
make

sudo insmod chardev.ko
sudo chmod 666 /dev/accelerator

./test_accel
python3 test_accel.py

sudo rmmod chardev
```

## Connection to HPC and Canonical

Real GPU drivers (NVIDIA, AMD) and hardware
accelerators used in HPC clusters follow this
exact character device pattern. Understanding
this interface is foundational for:
- Partner engineering with silicon vendors
- HPC accelerator support at NVIDIA
- Any low-level hardware enablement work
