export ARCH:=$(shell /usr/bin/dpkg-architecture -qDEB_HOST_ARCH_CPU)
export TARGET_OS:=${HOME}/lophilo.nfs
export TOOLCHAIN_PATH:=${HOME}/lophilo/codesourcery/arm926ej-s
export PATH:=${TOOLCHAIN_PATH}:${PATH}
export BUILD_DIR:=${HOME}/lophilo/obj/linux-debug 

obj-m += lophilo.o

.PHONY: clean install checkstyle

lophilo_arm.ko: lophilo.c
	make V=1 ARCH=arm -C ${BUILD_DIR} M=$(PWD) modules

clean: 
	make V=1 ARCH=arm -C ${BUILD_DIR} M=$(PWD) clean

install: lophilo_arm.ko
	sudo cp lophilo.ko ${TARGET_OS} 
	@echo "on the target system (zImage-debug):"
	@echo " mount -t debugfs none /sys/kernel/debug"
	@echo " rmmod lophilo;insmod /lophilo.ko"
	@echo " ls /sys/kernel/debug/lophilo"
	@echo " cat /proc/iomem | grep Lophilo"

checkstyle:
	# the root requirement doesn't seem to be there in 3.4
	${BUILD_DIR}/scripts/checkpatch.pl --root ~/lophilo/upstream/linux/ -f lophilo.c
