#ifndef SLAB_ALLOCATOR_H
#define SLAB_ALLOCATOR_H

#include <stddef.h>

void  *slab_alloc(size_t size);
void   slab_free(void *ptr);
void  *slab_realloc(void *ptr, size_t new_size);
void   slab_stats(void);

#endif
