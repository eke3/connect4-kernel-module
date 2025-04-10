obj-m += fourinarow.o

KDIR := /lib/modules/$(shell uname -r)/build
CFLAGS_fourinarow.o := -Wno-unused-result

all:
	make -C $(KDIR) M=$(PWD) modules

.PHONY: ins rm clean

ins:
	sudo insmod fourinarow.ko
	sudo chmod 666 /dev/fourinarow

rm:
	rmmod fourinarow

clean:
	make -C $(KDIR) M=$(PWD) clean
