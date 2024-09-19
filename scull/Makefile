# Slightly modified version of the makefile present in LDD3.
# “Linux Device Drivers, Third Edition, by Jonathan Corbet, Alessandro Rubini, and Greg Kroah-Hartman. Copyright
# 2005 O’Reilly Media, Inc., 0-596-00590-3."
ifneq ($(KERNELRELEASE),)
	obj-m := scull.o

else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.symvers *.order *.mod.o *.mod

endif
