ifneq ($(KERNELRELEASE),)
	obj-m += hw1.o
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	rm -rf *.o *.ko *.mod.c
endif
