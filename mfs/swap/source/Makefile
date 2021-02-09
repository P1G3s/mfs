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

insert:
	(insmod ./$(TARGET_MODULE).ko)

load: insert 
	$(eval SWAP_MAJOR_NUMBER := $(shell cat /proc/devices | grep SWAP_DRIVER | tr -d -c 0-9))
	(mknod /dev/swap_device c $(SWAP_MAJOR_NUMBER) 0) && (chmod 666 /dev/swap_device)

unload:
	(rmmod $(TARGET_MODULE)) && (rm /dev/swap_device)

endif
