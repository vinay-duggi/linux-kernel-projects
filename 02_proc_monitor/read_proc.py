#!/usr/bin/env python3
"""
Reads from our custom /proc/sysmonitor kernel module
Demonstrates the userspace side of the kernel-userspace interface

This is the same pattern as Linux system monitor
but instead of reading /proc/meminfo (kernel built-in)
we're reading /proc/sysmonitor (our own kernel module)
"""

def read_kernel_metrics():
    metrics = {}

    with open('/proc/sysmonitor', 'r') as f:
        for line in f:
            key, value = line.strip().split(': ', 1)
            metrics[key] = value

    return metrics

def display_metrics(metrics):
    mem_total = int(metrics['mem_total_kb']) / 1024
    mem_used  = int(metrics['mem_used_kb']) / 1024
    mem_free  = int(metrics['mem_free_kb']) / 1024
    mem_pct   = (int(metrics['mem_used_kb']) /
                 int(metrics['mem_total_kb'])) * 100

    print("=== System Monitor (via kernel module) ===")
    print(f"Kernel:   {metrics['kernel_version']}")
    print(f"Uptime:   {metrics['uptime']}")
    print(f"CPUs:     {metrics['cpu_count']}")
    print(f"Memory:   {mem_used:.1f}MB used / "
          f"{mem_total:.1f}MB total ({mem_pct:.1f}%)")
    print(f"Free:     {mem_free:.1f}MB")

if __name__ == '__main__':
    metrics = read_kernel_metrics()
    display_metrics(metrics)
