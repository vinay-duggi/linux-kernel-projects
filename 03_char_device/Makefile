obj-m += chardev.o


KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)


all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	gcc -o test_accel test_accel.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f test_accel

.PHONY: all clean