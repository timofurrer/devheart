obj-m += devheart.o
devheart-y = src/devheart.o
devheart-y += src/left_ventricle_beat.o
devheart-y += src/right_ventricle_beat.o

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
