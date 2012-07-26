export LOPHILO_DIR:=${HOME}/lophilo
export ARCH:=$(shell /usr/bin/dpkg-architecture -qDEB_HOST_ARCH_CPU)
export TARGET_OS:=${HOME}/lophilo.nfs
export TOOLCHAIN_PATH:=${LOPHILO_DIR}/codesourcery/arm926ej-s
export PATH:=${TOOLCHAIN_PATH}:${PATH}
export BUILD_DIR:=${LOPHILO_DIR}/obj/linux-debug
export SRC_DIR:=${LOPHILO_DIR}/linux

obj-m += lophilo.o

.PHONY: clean install checkstyle

lophilo.ko: lophilo.c
	make V=1 ARCH=arm -C ${BUILD_DIR} M=$(PWD) modules

clean: 
	make V=1 ARCH=arm -C ${BUILD_DIR} M=$(PWD) clean

install: lophilo.ko
	sudo cp lophilo.ko ${TARGET_OS} 

checkstyle:
	# the root requirement doesn't seem to be there in 3.4
	${SRC_DIR}/scripts/checkpatch.pl -f lophilo.c
