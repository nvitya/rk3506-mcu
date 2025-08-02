obj-m := rk3506_rproc.o

ARCH_ARGS := ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-

KDIR := ./kernel-source/
PWD := $(shell pwd)

all:
	$(MAKE) $(ARCH_ARGS) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
