export ARCH:=$(shell /usr/bin/dpkg-architecture -qDEB_HOST_ARCH_CPU)
obj-m += lophilo.o
MODULE_SUBDIR=$(TARGETDIR)/lib/modules/$(shell uname -r)
BUILD_DIR=/lib/modules/$(shell uname -r)/build

.PHONY: load clean install checkstyle

all: lophilo.ko /dev/lophilo load

lophilo.ko: lophilo.c
	make -C ${BUILD_DIR} M=$(PWD) modules

load: lophilo.ko
	-sudo rmmod lophilo
	sudo insmod lophilo.ko

clean:
	make -C ${BUILD_DIR} M=$(PWD) clean

install: lophilo.ko
	mkdir -p $(MODULE_SUBDIR)
	mkdir -p $(TARGETDIR)/bin
	cp lophilo.ko $(MODULE_SUBDIR)

checkstyle:
	# the root requirement doesn't seem to be there in 3.4
	${BUILD_DIR}/scripts/checkpatch.pl --root ~/lophilo/upstream/linux/ -f lophilo.c
