# Ensure the user provides a module name; stop otherwise
ifeq ($(MODULE_NAME),)
$(error MODULE_NAME is not set. Please provide it using 'make MODULE_NAME=<module_name>')
endif

ifneq ($(KERNELRELEASE),)
	obj-m := $(MODULE_NAME).o

else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)/$(MODULE_NAME)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	rm -rf $(PWD)/*.o $(PWD)/*~ $(PWD)/core $(PWD)/.depend $(PWD)/.*.cmd $(PWD)/*.ko \
	       $(PWD)/*.mod.c $(PWD)/.tmp_versions $(PWD)/*.symvers $(PWD)/*.order $(PWD)/*.mod.o $(PWD)/*.mod
endif
