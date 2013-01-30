export LOPHILO_DIR:=${HOME}/lophilo
export ARCH:=$(shell /usr/bin/dpkg-architecture -qDEB_HOST_ARCH_CPU)
export TARGET_OS:=${HOME}/lophilo.nfs
export TOOLCHAIN_PATH:=${LOPHILO_DIR}/codesourcery/arm926ej-s
export PATH:=${TOOLCHAIN_PATH}:${PATH}
export BUILD_DIR:=${LOPHILO_DIR}/obj/linux
export BUILD_DIR_DEBUG:=${LOPHILO_DIR}/obj/linux-debug
export SRC_DIR:=${LOPHILO_DIR}/linux
export KBUILD_VERBOSE:=0
#export KV:=$(shell make -C ${SRC_DIR} -f ${SRC_DIR}/Makefile kernelversion)

#TODO: figure out how to extract this from obj directory
export MODULE_VERSION:=3.4.18+
export MODULE_VERSION_DEBUG:=3.4.18-debug+
export MODULE_PATH:=${TARGET_OS}/lib/modules/${MODULE_VERSION}
export MODULE_PATH_DEBUG:=${TARGET_OS}/lib/modules/${MODULE_VERSION_DEBUG}
export MODULE_PATH_LOPHILO:=${MODULE_PATH}/lophilo
export MODULE_PATH_LOPHILO_DEBUG:=${MODULE_PATH_DEBUG}/lophilo

obj-m += lophilo.o

.PHONY: clean install checkstyle

all: install

lophilo-standard.ko: lophilo.c ${BUILD_DIR}/arch/arm/boot/zImage
	make ARCH=arm -C ${BUILD_DIR} M=$(PWD) modules
	mv lophilo.ko lophilo-standard.ko

lophilo-debug.ko: lophilo.c ${BUILD_DIR_DEBUG}/arch/arm/boot/zImage
	make ARCH=arm -C ${BUILD_DIR_DEBUG} M=$(PWD) modules
	mv lophilo.ko lophilo-debug.ko

clean: 
	make ARCH=arm -C ${BUILD_DIR} M=$(PWD) clean

install: ${MODULE_PATH}/modules.dep ${MODULE_PATH_DEBUG}/modules.dep

${MODULE_PATH}/modules.dep: ${MODULE_PATH_LOPHILO}/lophilo.ko
	@echo "don't forget to run depmod -a on the target system to generate $@"

${MODULE_PATH_DEBUG}/modules.dep: ${MODULE_PATH_LOPHILO_DEBUG}/lophilo.ko
	@echo "don't forget to run depmod -a on the target system $@"

${MODULE_PATH_LOPHILO_DEBUG}/lophilo.ko: lophilo-debug.ko
	sudo mkdir -p ${MODULE_PATH_LOPHILO_DEBUG}
	sudo cp lophilo-debug.ko ${MODULE_PATH_LOPHILO_DEBUG}

${MODULE_PATH_LOPHILO}/lophilo.ko: lophilo-standard.ko
	sudo mkdir -p ${MODULE_PATH_LOPHILO}
	sudo cp lophilo-standard.ko ${MODULE_PATH_LOPHILO}/lophilo.ko

checkstyle:
	# the root requirement doesn't seem to be there in 3.4
	${SRC_DIR}/scripts/checkpatch.pl -f lophilo.c

lophilo_user: lophilo_user.c
	gcc -g lophilo_user.c -o lophilo_user
