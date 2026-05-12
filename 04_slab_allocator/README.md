# Slab Memory Allocator

User space implementation of a slab-style memory allocator
demonstrating the concepts used in the Linux kernel's
memory management subsystem.

## What This Demonstrates

**Memory allocation concepts:**
- Free list management with doubly linked blocks
- Block splitting - divide large blocks for small requests
- Block coalescing - merge adjacent free blocks
- Alignment - all allocations 8-byte aligned
- Magic numbers - detect double free and corruption

**Linux kernel connection:**
The Linux kernel uses a slab allocator (`kmalloc`)
for kernel memory management. Key concepts:
- Slabs are pools of same-sized objects
- Reduces fragmentation vs raw page allocation
- Cache-friendly - objects stay in same memory region
- This implementation demonstrates the same principles

**Algorithms implemented:**
- First-fit allocation - find first block large enough
- Block splitting - avoid wasting memory
- Coalescing - merge free neighbors to reduce fragmentation
- Magic number validation - detect memory corruption

## Build and Run

```bash
make
./test_allocator
```

## Test Suite
Test 1: Basic alloc/free
Test 2: Memory reuse after free
Test 3: Coalescing adjacent blocks
Test 4: 8-byte alignment guarantee
Test 5: Realloc with data preservation
Test 6: Double free detection
Test 7: Stress test - 1000 allocations

## Connection to Linux Kernel

The Linux kernel slab allocator (`mm/slab.c`):
- `kmalloc()` - equivalent to our `slab_alloc()`
- `kfree()`   - equivalent to our `slab_free()`
- `krealloc()`- equivalent to our `slab_realloc()`

Understanding this user space version makes
reading kernel memory management source code
significantly easier.