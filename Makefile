obj-m += devheart.o
devheart-y = src/devheart.o
devheart-y += src/single_beat.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD)

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

insert:
	insmod devheart.ko

remove:
	rmmod devheart.ko
