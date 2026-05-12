# /proc Filesystem Kernel Module

Creates a custom `/proc/sysmonitor` entry that exposes
system metrics directly from the Linux kernel.

## What This Demonstrates

**VFS and /proc filesystem:**
The Linux Virtual File System (VFS) allows kernel modules
to create files under /proc that expose kernel data
to userspace. Reading `/proc/sysmonitor` triggers
our kernel function directly - no actual file on disk.

**Kernel APIs used:**
- `si_meminfo()` - reads memory statistics from kernel
- `jiffies` - kernel time counter since boot
- `num_online_cpus()` - CPU count from kernel
- `seq_file` - safe sequential file interface for /proc
- `proc_ops` - /proc file operations structure

**Connection to Linux System Monitor project:**
Our userspace system monitor reads `/proc/meminfo`
and `/proc/stat` - kernel built-in /proc files.
This module creates our OWN /proc entry using the
same mechanism the kernel uses internally.

## Build and Run

```bash
# Build
make

# Load module - creates /proc/sysmonitor
sudo insmod proc_monitor.ko

# Read from shell
cat /proc/sysmonitor

# Read from Python
python3 read_proc.py

# Unload - removes /proc/sysmonitor
sudo rmmod proc_monitor
```

## Output
kernel_version: 6.17.0-14-generic
uptime: 4:12:10
cpu_count: 4
mem_total_kb: 3991132
mem_free_kb: 913636
mem_available_kb: 993648
mem_used_kb: 3077496
swap_total_kb: 0
swap_free_kb: 0

## Why /proc Matters

Every tool you use reads /proc:
- `top` and `htop` read `/proc/stat`, `/proc/meminfo`
- `ps` reads `/proc/[pid]/status`
- `free` reads `/proc/meminfo`
- `uptime` reads `/proc/uptime`

This module creates our own entry using the exact same kernel mechanism.