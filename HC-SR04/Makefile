# Makefile for the DS18B20 drivers.

obj-m += HCSR04_drv.o

#directory of the kernel code
KERNELDIR:=/home/yliu/6818/kernel
#cross compile toolchain
CROSS_COMPILE:=arm-eabi-

#get the full pathname
PWD:=$(shell pwd)

default:
	$(MAKE) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	rm -rf *.o *.order .*.cmd *.ko *.mod.c *.symvers *.tmp_versions
