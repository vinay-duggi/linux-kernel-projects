CC      = gcc
CFLAGS  = -Wall -Wextra -g -fsanitize=address

all: test_allocator

test_allocator: slab_allocator.c
	$(CC) $(CFLAGS) -o test_allocator slab_allocator.c

clean:
	rm -f test_allocator

.PHONY: all clean
