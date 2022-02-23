# obj-m += usbmouse_base.o
# obj-m += usbmouse_chrdev.o
# obj-m += usbmouse_full.o
# obj-m += usbmouse_info.o
# obj-m += usbmouse_func.o 
# obj-m += usbmouse_dev.o
obj-m += usbmouse_read.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

modules_install:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules_install