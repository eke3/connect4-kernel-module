obj-m += fourinarow.o

KDIR := /lib/modules/$(shell uname -r)/build
# Escape the space in PWD
PWD := $(shell pwd | sed 's/ /\\ /g')

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
