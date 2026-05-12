# Kernel Module Makefile
#
# obj-m - object file to build as module (m = module)
# obj-y would mean build into kernel directly
obj-m += proc_monitor.o

# KDIR - path to kernel build directory
# Contains kernel headers and build system
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd) # PWD - current directory where our module lives

# Default target - build the module
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Clean build artifacts
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# Show module info after building
info:
	modinfo proc_monitor.ko

.PHONY: all clean info