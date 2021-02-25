TARGET_MODULE:=hide_driver
ifneq ($(KERNELRELEASE),)
	$(TARGET_MODULE)-objs := hide_device.o hide_module.o 
	obj-m := $(TARGET_MODULE).o

else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
#KERNELDIR ?= /lib/modules/5.4.0-66-generic/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

insert:
	(insmod ./$(TARGET_MODULE).ko) && (cat /proc/devices | grep HIDE_DRIVER)

load: insert
	$(eval MAJOR_NUMBER := $(shell cat /proc/devices| grep HIDE_DRIVER | tr -d -c 0-9))
	(mknod /dev/hide_device c $(MAJOR_NUMBER) 0) && (chmod 666 /dev/hide_device)

unload:
	(rmmod $(TARGET_MODULE)) && (rm /dev/hide_device)

endif 
