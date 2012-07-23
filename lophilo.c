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
#include <linux/ioport.h>
#include <asm/io.h>
#include <asm/uaccess.h>  /* for put_user */

#define MAX_SUBSYSTEMS 16

struct subsystem {
	u32 id;
	u32 size;
	char* addr;
	u32 current_offset;
};

static struct subsystem subsystems[MAX_SUBSYSTEMS];

struct resource * fpga;

const u32 FPGA_BASE_ADDR = 0x10000000;
const int SIZE16MB = 16777216; // 2^24
void* identifier;
const u32 RGB_VALUE = 0x00006015;

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static int Device_Open = 0;  /* Is device open?  Used to prevent multiple
                                        access to the device */
static struct dentry *lophilo_dentry;

struct file_operations fops = {
       .read = device_read,
       .write = device_write,
       .open = device_open,
       .release = device_release
 };

static int __init
lophilo_init(void)
{
	struct dentry *lophilo_dentry_test;
	static struct dentry *lophilo_subsystem_dentry;
	int subsystem_id = 0;
	char subsystem_str[64];

	printk(KERN_INFO "Lophilo module loading\n");

	lophilo_dentry = debugfs_create_dir(
		"lophilo",
		NULL);

	//fpga = request_mem_region(FPGA_BASE_ADDR, SIZE16MB, "Lophilo FPGA LEDs");
	identifier = ioremap_nocache(FPGA_BASE_ADDR, SIZE16MB);

	//iowrite8(0x0, identifier + 0x103);
	//iowrite8(0x0, identifier + 0x102);
	//iowrite8(0x60, identifier + 0x101);
	//iowrite8(0x15, identifier + 0x100);

	while(true) {
		if(subsystem_id == MAX_SUBSYSTEMS) {
			printk(KERN_INFO "Lophilo ended detection, maximum found %ld\n", MAX_SUBSYSTEMS);
			break;
		}

		subsystems[subsystem_id].addr = identifier;
		subsystems[subsystem_id].id = ioread32(identifier + 0x4);

		if((subsystems[subsystem_id].id & 0xea000000) == 0xea000000) {
			printk(KERN_INFO "Lophilo adding subsystem 0x%x of type 0x%x\n", subsystem_id, subsystems[subsystem_id].id);
		} else {
			printk(KERN_INFO "Lophilo ended detection, found 0x%x\n", subsystems[subsystem_id].id);
			break;
		}


		subsystems[subsystem_id].size = ioread32(identifier);
		printk(KERN_INFO "Subsystem size 0x%x\n", subsystems[subsystem_id].size);

		sprintf(subsystem_str, "subsystem%d", subsystem_id);
		lophilo_subsystem_dentry = debugfs_create_dir(
			subsystem_str,
			lophilo_dentry);

		lophilo_dentry_test = debugfs_create_x32(
			"size",
			S_IRWXU | S_IRWXG | S_IRWXO,
			lophilo_subsystem_dentry,
			&subsystems[subsystem_id].size);

		lophilo_dentry_test = debugfs_create_x32(
			"id",
			S_IRWXU | S_IRWXG | S_IRWXO,
			lophilo_subsystem_dentry,
			&subsystems[subsystem_id].id);

		lophilo_dentry_test = debugfs_create_x32(
			"addr",
			S_IRWXU | S_IRWXG | S_IRWXO,
			lophilo_subsystem_dentry,
			&subsystems[subsystem_id].addr);

		lophilo_dentry_test = debugfs_create_file(
			"mem",
			S_IRWXU | S_IRWXG | S_IRWXO,
			lophilo_subsystem_dentry,
			&subsystems[subsystem_id],
			&fops
			);
		identifier += subsystems[subsystem_id].size;
		printk(KERN_INFO "identifier increment to 0x%x for subsystem 0x%x", identifier, subsystem_id);
		subsystem_id++;

	}

	return 0;
}

void __exit
lophilo_cleanup(void)
{
	printk(KERN_INFO "Lophilo module uninstalling\n");
	debugfs_remove_recursive(lophilo_dentry);
	iounmap(identifier);
	//release_mem_region(FPGA_BASE_ADDR, SIZE16MB);
	return;
}

/* Methods */

/* Called when a process tries to open the device file, like
 * "cat /dev/mycharfile"
 */
static int device_open(struct inode *inode, struct file *file)
{
   struct subsystem* subsystem_ptr = inode->i_private;

   if (Device_Open)
   	return -EBUSY;
   Device_Open++;

   printk(KERN_INFO "Dumping memory of subsystem size 0x%x bytes\n", subsystem_ptr->size);

   subsystem_ptr->current_offset = 0;
   file->private_data = subsystem_ptr;

   return 0;
}


/* Called when a process closes the device file */
static int device_release(struct inode *inode, struct file *file)
{
   Device_Open --;     /* We're now ready for our next caller */

   return 0;
}

/* Called when a process, which already opened the dev file, attempts to
   read from it.
*/
static ssize_t device_read(struct file *filp,
   char *buffer,    /* The buffer to fill with data */
   size_t length,   /* The length of the buffer     */
   loff_t *offset)  /* Our offset in the file       */
{
   struct subsystem* subsystem_ptr = filp->private_data;

   /* Number of bytes actually written to the buffer */
   int bytes_read = 0;

   if(subsystem_ptr->current_offset >= subsystem_ptr->size)
   	return 0;

   /* Actually put the data into the buffer */
   while (length && (subsystem_ptr->current_offset < subsystem_ptr->size))  {

        /* The buffer is in the user data segment, not the kernel segment;
         * assignment won't work.  We have to use put_user which copies data from
         * the kernel data segment to the user data segment. */
         put_user(subsystem_ptr->addr[subsystem_ptr->current_offset], buffer++);
         subsystem_ptr->current_offset++;

         length--;
         bytes_read++;
   }

   /* Most read functions return the number of bytes put into the buffer */
   return bytes_read;
}


/*  Called when a process writes to dev file: echo "hi" > /dev/hello */
static ssize_t device_write(struct file *filp,
   const char *buff,
   size_t len,
   loff_t *off)
{
   printk ("<1>Sorry, this operation isn't supported.\n");
   return -EINVAL;
}

MODULE_LICENSE("GPL");

module_init(lophilo_init);
module_exit(lophilo_cleanup);
