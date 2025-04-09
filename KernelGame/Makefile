obj-m += fourinarow.o

KDIR := /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules
	sudo su

.PHONY: ins rm clean

ins:
	insmod fourinarow.ko
	chmod a+r /dev/fourinarow

rm:
	rmmod fourinarow

clean:
	make -C $(KDIR) M=$(PWD) clean
