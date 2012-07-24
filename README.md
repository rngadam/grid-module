# Cross-compiled Linux kernel driver module support

Check that the module matches kernel style:

	make checkstyle

Install to the target directory (recommend NFS directory mountable by Lophilo):

	make install

On the target system (using the zImage-debug; zImage does not have debugfs), mount debugfs:

	mount -t debugfs none /sys/kernel/debug

Reload module:

	rmmod lophilo;insmod /lophilo.ko

Check discovered subsystems:

	tree /sys/kernel/debug/lophilo

There are a couple of debugfs files for each subsystem:

* size: the size of the memory registers
* id: the id of the subsystem
* addr: it's iomapped memory 
* mem: read the whole area 
	(TODO: write to it directly; use debugfs register functions? )

TODO: this should have the I/O reservations

	cat /proc/iomem | grep Lophilo
