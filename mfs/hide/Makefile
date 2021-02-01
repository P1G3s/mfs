TARGET_MODULE:=hide_driver
ifneq ($(KERNELRELEASE),)
	$(TARGET_MODULE)-objs := hide_device.o hide_module.o 
	obj-m := $(TARGET_MODULE).o

else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

load:
	insmod ./$(TARGET_MODULE).ko
	(lsmod | grep hide_driver) && (mknod /dev/hide_device c 240 0) && (chmod 666 /dev/hide_device)


unload:
	(rmmod $(TARGET_MODULE)) && (rm /dev/hide_device)

endif
