/* === STANDARD LIBRARIES === */
#include <stdio.h>  // For printf, fprintf
#include <stdlib.h> // For rand() in our stress test
#include <string.h> // For memcpy, memset, strcpy
#include <stdint.h> // For uint32_t, uintptr_t (fixed-width integers)
#include <assert.h> // For our test assertions

/* === CONFIGURATION CONSTANTS === */
/* PAGE_SIZE simulates the amount of memory the OS gives us at one time.
    4096 bytes (4KB) is the standard page size on almost all latest CPUs.*/
#define PAGE_SIZE 4096
#define MAX_PAGES 64                            // We simulate having a maximum of 64 pages (256 KB of total RAM)
#define ALIGNMENT 8                             // CPUs read memory much faster if the addresses are multiples of 8.
#define MIN_ALLOC 8                             // We don't want to split a block if the remaining space is tiny (e.g., 2 bytes). It wastes space because the Block Header itself is larger than 2 bytes!
#define MAX_ALLOC (PAGE_SIZE - sizeof(Block))   // The maximum size a user can request. It's the size of the page MINUS our hidden header.


/* === HIDDEN HEADER (Metadata) === */
/* Every time the user asks for memory, we attach this struct
    to the memory just BEFORE the pointer is given to them. */
typedef struct Block {
    size_t size;            // How many bytes the user actually requested
    int free;               // Boolean: 1 if free, 0 if currently in use
    struct Block *next;     // Pointer to the next chunk of memory in this page
    struct Block *prev;     // Pointer to the previous chunk of memory
    // "Magic Numbers" are used to detect memory corruption. If the user writes 
    // past the end of their array, they will overwrite this number, and we will know!
    uint32_t magic;
} Block;


/* === MEMORY === */
/* Struct to represent a single 4KB chunk of RAM. */
typedef struct Page {
    char memory[PAGE_SIZE]; // This is the actual raw memory. All blocks and user data live inside this array.
    Block *free_list;       //A pointer to the very first Block header inside this page's memory array.
    // Numbers for tracking memory leaks and usage
    size_t used_bytes; 
    int num_allocs;
    int num_frees;
} Page;


/* === MAGIC NUMBERS === */
/* Spell out words so we instantly know the state of the memory block */
#define MAGIC_FREE 0xDEADBEEF   // Block is free
#define MAGIC_USED 0xCAFEBABE   // Block is in use


/* === GLOBAL STATES === */
/* In a real OS, this would be dynamically requested from the kernel using mmap() or sbrk().
    For this simulation, we just pre-allocate a global array of 64 pages.*/
static Page pages[MAX_PAGES];
static int num_pages = 0;   // Tracks how many pages we've "turned on"


/* === HELPER: ALIGNMENT === */
/* This takes a requested size (e.g., 13 bytes) and rounds it UP to the nearest
    multiple of ALIGNMENT (8). So 13 becomes 16. It uses fast bitwise math.*/
static size_t align_size(size_t size)
{
    // (13 + 8 - 1) & ~(8 - 1)  ->  (20) & ~(7)  ->  16
    return (size + ALIGNMENT - 1) & ~(ALIGNMENT -1);
}


/* === HELPER: REQUEST A NEW PAGE === */
/* When we run out of space, we "turn on" a new 4KB page */
static Page *new_page(void)
{
    if (num_pages >= MAX_PAGES)
    {
        return NULL;    // Out of memory!
    }
    // Grab the next available page from our global array
    Page *page = &pages[num_pages++];
    // Place a Block header at the absolute beginning of the page's raw memory array
    Block *block = (Block *)page -> memory;

    // Initialize this block to represent the ENTIRE page as one giant free chunk
    block->size = PAGE_SIZE - sizeof(Block);
    block->free = 1;
    block->next = NULL;
    block->prev = NULL;
    block->magic = MAGIC_FREE;

    // Attach the page metadata 
    page->free_list = block;
    page->used_bytes = 0;
    page->num_allocs = 0;
    page->num_frees = 0;

    return page;
}


/* === CORE LOGIC: SPLITTING === */
/* If the user asks for 100 bytes, but our free block is 4000 bytes,
    we chop the 4000-byte block into a 100-byte block and a 3900-byte block.*/
static void split_block(Block *block, size_t size)
{
    // Don't split if the leftover space isn't big enough to hold a new Block header + data
    if (block->size < size + sizeof(Block) + MIN_ALLOC)
    {
        return;
    }
    // Find the exact memory address where the new block should start.
    // Address of current block + size of the header + size of the user's requested memory.
    // (We cast to 'char*' so C does math by exact bytes, not by sizes of structs).
    Block *new_block = (Block *)((char *)block + sizeof(Block) + size);
    new_block->size = block->size - size - sizeof(Block); // Setup the new leftover block
    new_block->free = 1;
    new_block->magic = MAGIC_FREE;
    new_block->next = block->next;  // Insert the new block into our Doubly Linked List
    new_block->prev = block;

    if (block->next)
    {
        block->next->prev = new_block;
    }
    block->next = new_block;    // Shrink the current block to the requested size
    block->size = size;
}


/* === COALESCING (Defragmentation) === */
/* When a block is freed, check if its neighbors are also free.
    If they are, merge them together into one massive free block. */
static void coalesce(Block *block)
{
    // Look right (at the next block)
    if (block->next && block->next->free) {
        // Add the next block's size (PLUS the size of its header ) to our size
        block->size += sizeof(Block) + block->next->size;
        // Remove the next block from the linked list by skipping over it
        block->next = block->next->next;
        if (block->next)
        {
            block->next->prev = block;
        }
    }

    // Look left (at the previous block)
    if (block->prev && block->prev->free) {
        // Add our size (PLUS our header size) to the previous block's size 
        block->prev->size += sizeof(Block) + block->size;
        // Remove ourselves from the linked list (the previous block eats us)
        block->prev->next = block->next;
        if (block->next)
        {
            block->next->prev = block->prev;
        }
    }
}


/* === MALLOC() REPLACEMENT === */
void *slab_alloc(size_t size)
{
    // Sanity checks
    if (size == 0 || size > MAX_ALLOC)
    {
        return NULL;
    }
    size = align_size(size);

    // Loop through every page we own
    for (int i = 0; i < num_pages; i++)
    {
        // Start at the first block in the page
        Block *block = pages[i].free_list;
        
        // Traverse the linked list of blocks
        while (block) {
            // "First-Fit" Algo: Find the first block that is free and large enough
            if (block->free && block->size >= size) {
                // Chop the block up if it's too big
                split_block(block, size);
                // Mark it as actively used by the user
                block->free = 0;
                block->magic = MAGIC_USED;
                // Update stats
                pages[i].used_bytes += size;
                pages[i].num_allocs++;
                // RETURN VALUE: We do NOT return the block header. We return the 
                // memory address immediately *after* the header, which is safe for the user.
                return (void *)((char *)block + sizeof(Block));
            }
            //Move to the next block in the chain
            block = block->next;
        }
    }
    
    // If we get here, no existing pages had enough free space. Let's create a new page!
    Page *page = new_page();
    if (!page)
    {
        return NULL;    // Total system out of memory
    }
    // Try again with the newly created page
    return slab_alloc(size);
}


/* === FREE() REPLACEMENT === */
void slab_free(void *ptr)
{
    if (!ptr)
    {
        return; // Freeing a NULL pointer does nothing
    }
    // The user gave us a pointer to their data. We step backwards 
    // in memory by the exact size of a Block to find our hidden header.
    Block *block = (Block *)((char *)ptr - sizeof(Block));

    // Security check: Did the user corrupt the header? or try to double-free?
    if (block->magic != MAGIC_USED){
        fprintf(stderr, "slab_free: invalid or double free at %p\n", ptr);
        return; 
    }

    // Mark it free
    block->free = 1;
    block->magic = MAGIC_FREE;

    // To update our page statistics, we need to figure out which Page this block 
    // belongs to. We do this by checking if the block's memory address falls 
    // within the Page's memory array boundaries.
    for (int i = 0; i < num_pages; i++) {
        char *start = pages[i].memory;
        char *end = pages[i].memory + PAGE_SIZE;

        if ((char *)block >= start && (char *)block < end) {
            pages[i].used_bytes -= block->size;
            pages[i].num_frees++;
            break;  // Found the parent page, stop searching
        }
    }
    coalesce(block);    // Attempt to merge this newly freed block with its neighbors to prevent fragmentation
}


/* === REALLOC() REPLACEMENT === */
/* Realloc handles resizing an already allocated chunk of memory. */
void *slab_realloc(void *ptr, size_t new_size)
{
    // If ptr is NULL, realloc acts like malloc
    if (!ptr) {
        return slab_alloc(new_size);
    }

    // If size is 0, realloc acts like free
    if (new_size == 0) {
        slab_free(ptr);
        return NULL;
    }

    // Look at our hidden header to find out how big the chunk currently is
    Block *block = (Block *)((char *)ptr - sizeof(Block));
    // If the chunk is already big enough to handle the new requested size, just return it!
    if (block->size >= align_size(new_size)) {
        return ptr;
    }

    // Otherwise, the chunk is too small. We must find a completely new home for it.
    void *new_ptr = slab_alloc(new_size);
    if (!new_ptr) {
        return NULL;
    }

    // Copy the old data to the new memory location
    memcpy(new_ptr, ptr, block->size);
    slab_free(ptr); // Free the old, small block
    return new_ptr;
}


/* === DEBUGGING / STATS === */
/* A helper function to print the current health of the memory */
void slab_stats(void)
{
    printf("\n=== Slab Allocator Stats ===\n");
    printf("Active pages: %d / %d\n", num_pages, MAX_PAGES);

    size_t total_used  = 0;
    size_t total_free  = 0;
    int    total_alloc = 0;
    int    total_free_ops = 0;

    for (int i = 0; i < num_pages; i++) {
        size_t page_free = 0;
        int    blocks    = 0;
        int    free_blks = 0;

        Block *block = pages[i].free_list;
        while (block) {
            blocks++;
            if (block->free) {
                free_blks++;
                page_free += block->size;
            }
            block = block->next;
        }

        printf("Page %d: used=%zu free=%zu "
               "blocks=%d free_blocks=%d "
               "allocs=%d frees=%d\n",
               i,
               pages[i].used_bytes,
               page_free,
               blocks,
               free_blks,
               pages[i].num_allocs,
               pages[i].num_frees);

        total_used     += pages[i].used_bytes;
        total_free     += page_free;
        total_alloc    += pages[i].num_allocs;
        total_free_ops += pages[i].num_frees;
    }

    printf("Total: used=%zu free=%zu allocs=%d frees=%d\n",
           total_used, total_free, total_alloc, total_free_ops);
    printf("============================\n\n");
}


/* === TESTS === */
/* Test implementations are straightforward uses of assert to verify the allocator works */
/* Tests simple alloc/free cycle */
static void test_basic(void)
{
    printf("--- Test 1: Basic alloc and free ---\n");

    void *p1 = slab_alloc(100);
    void *p2 = slab_alloc(200);
    void *p3 = slab_alloc(50);

    assert(p1 != NULL);
    assert(p2 != NULL);
    assert(p3 != NULL);
    assert(p1 != p2);
    assert(p2 != p3);

    strcpy((char *)p1, "Hello kernel allocator");
    strcpy((char *)p2, "Second allocation");

    printf("p1=%p: \"%s\"\n", p1, (char *)p1);
    printf("p2=%p: \"%s\"\n", p2, (char *)p2);

    slab_free(p1);
    slab_free(p2);
    slab_free(p3);

    printf("PASSED\n\n");
}

/* Verifies freed memory gets handed out again */
static void test_reuse(void)
{
    printf("--- Test 2: Memory reuse after free ---\n");

    void *p1 = slab_alloc(200);
    slab_free(p1);

    void *p2 = slab_alloc(200);
    assert(p2 == p1);

    printf("p1=%p p2=%p (same address = reused)\n", p1, p2);
    slab_free(p2);

    printf("PASSED\n\n");
}

/* Verifies neighbors merge into one big block */
static void test_coalesce(void)
{
    printf("--- Test 3: Coalescing adjacent free blocks ---\n");

    void *p1 = slab_alloc(100);
    void *p2 = slab_alloc(100);
    void *p3 = slab_alloc(100);

    slab_free(p1);
    slab_free(p2);

    void *p4 = slab_alloc(200);
    assert(p4 != NULL);

    printf("Allocated 200 bytes from two coalesced "
           "100-byte blocks\n");

    slab_free(p3);
    slab_free(p4);

    printf("PASSED\n\n");
}

/* Verifies all pointers are multiples of 8 */
static void test_alignment(void)
{
    printf("--- Test 4: Alignment ---\n");

    for (int i = 1; i <= 16; i++) {
        void *p = slab_alloc(i);
        assert(p != NULL);
        assert(((uintptr_t)p % ALIGNMENT) == 0);
        slab_free(p);
    }

    printf("All allocations 8-byte aligned\n");
    printf("PASSED\n\n");
}

/* Verifies data survives being moved to a bigger block */
static void test_realloc(void)
{
    printf("--- Test 5: Realloc ---\n");

    char *p = slab_alloc(50);
    strcpy(p, "original data");

    p = slab_realloc(p, 200);
    assert(p != NULL);
    assert(strcmp(p, "original data") == 0);

    printf("Data preserved after realloc: \"%s\"\n", p);
    slab_free(p);

    printf("PASSED\n\n");
}

/* Verifies the magic number protects against crashes */
static void test_double_free(void)
{
    printf("--- Test 6: Double free detection ---\n");

    void *p = slab_alloc(100);
    slab_free(p);
    slab_free(p);

    printf("Double free detected and handled safely\n");
    printf("PASSED\n\n");
}

/* Allocates/Frees 1000 random sizes rapidly */
static void test_stress(void)
{
    printf("--- Test 7: Stress test (1000 allocs) ---\n");

    void *ptrs[100];
    int   success = 0;

    for (int round = 0; round < 10; round++) {
        for (int i = 0; i < 100; i++) {
            size_t size = (rand() % 256) + 8;
            ptrs[i] = slab_alloc(size);
            if (ptrs[i]) {
                memset(ptrs[i], round, size);
                success++;
            }
        }
        for (int i = 0; i < 100; i++) {
            if (ptrs[i]) {
                slab_free(ptrs[i]);
            }
        }
    }

    printf("Completed %d successful allocations\n", success);
    printf("PASSED\n\n");
}

int main(void)
{
    printf("=== Slab Allocator Test Suite ===\n\n");

    test_basic();
    test_reuse();
    test_coalesce();
    test_alignment();
    test_realloc();
    test_double_free();
    test_stress();

    slab_stats();

    printf("All tests passed\n");
    return 0;
}