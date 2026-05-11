# Kernel Module Makefile
#
# obj-m — object file to build as module (m = module)
# obj-y would mean build into kernel directly
obj-m += hello_module.o

# KDIR — path to kernel build directory
# Contains kernel headers and build system
KDIR := /lib/modules/$(shell uname -r)/build

# PWD — current directory where our module lives
PWD := $(shell pwd)

# Default target — build the module
all:
	# $(MAKE) — invoke kernel build system
	# -C $(KDIR) — change to kernel build dir first
	# M=$(PWD) — come back to our dir to find source
	# modules — build target
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Clean build artifacts
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# Install module to system
install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	depmod -a

# Show module info after building
info:
	modinfo hello_module.ko

.PHONY: all clean install info
