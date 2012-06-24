/*
 * Demonstration of the use of debugfs
 *
 * Author: Ricky Ng-Adam <rngadam@lophilo.com>
 *
 * Usage example:
 *
 * $ sudo bash -c "echo 42 > /sys/kernel/debug/lophilo/test"
 * $ sudo bash -c "cat /sys/kernel/debug/lophilo/test"
 * 0x0000002a
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/debugfs.h>

struct dentry *lophilo_dentry;
struct dentry *lophilo_dentry_test;

const char *lophilo_dentry_name = "lophilo";
const char *lophilo_dentry_test_name = "test";

u32 test_value;


static int __init
lophilo_init(void)
{
	printk(KERN_DEBUG, "Lophilo module loading\n");
	lophilo_dentry = debugfs_create_dir(
		lophilo_dentry_name,
		NULL);

	lophilo_dentry_test = debugfs_create_x32(
		lophilo_dentry_test_name,
		S_IRWXU | S_IRWXG | S_IRWXO,
		lophilo_dentry,
		&test_value);

	return 0;
}

void __exit
lophilo_cleanup(void)
{
	printk(KERN_DEBUG, "Lophilo module uninstalling\n");
	debugfs_remove_recursive(lophilo_dentry);

	return;
}

MODULE_LICENSE("GPL");

module_init(lophilo_init);
module_exit(lophilo_cleanup);
