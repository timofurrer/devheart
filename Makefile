obj-m += src/devheart.o

MODULES_DIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(MODULES_DIR) M=$(PWD)

clean:
	$(MAKE) -C $(MODULES_DIR) M=$(PWD) clean

insert:
	insmod src/devheart.ko

remove:
	rmmod src/devheart.ko
