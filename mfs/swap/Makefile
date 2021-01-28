TARGET_MODULE:=swap_driver
ifneq ($(KERNELRELEASE),)
	$(TARGET_MODULE)-objs := swap_device.o swap_module.o 
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
	(lsmod | grep swap_driver) && (mknod /dev/swap_device c 240 0)


unload:
	(rmmod $(TARGET_MODULE)) && (rm /dev/swap_device)

endif
