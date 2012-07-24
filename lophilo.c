/*
 * Demonstration of the use of debugfs
 *
 * Author: Ricky Ng-Adam <rngadam@lophilo.com>
 *
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

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static int Device_Open = 0;  /* Is device open?  Used to prevent multiple
                                        access to the device */
static struct dentry *lophilo_dentry;

// from linux/arch/arm/mach-at91/board-tabby.c
extern void __iomem *fpga_cs0_base;
extern void __iomem *fpga_cs1_base;
extern void __iomem *fpga_cs2_base;
extern void __iomem *fpga_cs3_base;

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
	void* current_addr;
	int i;

	printk(KERN_INFO "Lophilo module loading\n");

	lophilo_dentry = debugfs_create_dir(
		"lophilo",
		NULL);
	if(lophilo_dentry == NULL) {
		printk(KERN_ERR "Could not create root directory entry lophilo in debugfs");
		return -EINVAL;
	}
	//fpga = request_mem_region(FPGA_BASE_ADDR, SIZE16MB, "Lophilo FPGA LEDs");
	// power on
	//iowrite32(0x03030300, fpga_cs0_base + 0x200);

	debugfs_create_x16(
		"id",
		S_IRWXU | S_IRWXG | S_IRWXO,
		lophilo_dentry,
		fpga_cs0_base + 0x0);
	debugfs_create_x16(
		"flag",
		S_IRWXU | S_IRWXG | S_IRWXO,
		lophilo_dentry,
		fpga_cs0_base + 0x2);
	debugfs_create_x32(
		"ver",
		S_IRWXU | S_IRWXG | S_IRWXO,
		lophilo_dentry,
		fpga_cs0_base + 0x4);
	debugfs_create_x32(
		"lock",
		S_IRWXU | S_IRWXG | S_IRWXO,
		lophilo_dentry,
		fpga_cs0_base + 0x8);
	debugfs_create_x32(
		"lockb",
		S_IRWXU | S_IRWXG | S_IRWXO,
		lophilo_dentry,
		fpga_cs0_base + 0xc);

	for(i=0; i<4; i++) {
		sprintf(subsystem_str, "led%d", i);
		lophilo_subsystem_dentry = debugfs_create_dir(
			subsystem_str,
			lophilo_dentry);

		debugfs_create_x8(
			"b",
			S_IRWXU | S_IRWXG | S_IRWXO,
			lophilo_subsystem_dentry,
			fpga_cs0_base + 0x100 + 0x4 * i);
		debugfs_create_x8(
			"g",
			S_IRWXU | S_IRWXG | S_IRWXO,
			lophilo_subsystem_dentry,
			fpga_cs0_base + 0x101 + 0x4 * i);
		debugfs_create_x8(
			"r",
			S_IRWXU | S_IRWXG | S_IRWXO,
			lophilo_subsystem_dentry,
			fpga_cs0_base + 0x102 + 0x4 * i);
		debugfs_create_x8(
			"s",
			S_IRWXU | S_IRWXG | S_IRWXO,
			lophilo_subsystem_dentry,
			fpga_cs0_base + 0x103 + 0x4 * i);
		debugfs_create_x32(
			"srgb",
			S_IRWXU | S_IRWXG | S_IRWXO,
			lophilo_subsystem_dentry,
			fpga_cs0_base + 0x100);
	}

	current_addr = fpga_cs1_base;

	while(true) {
		if(subsystem_id == MAX_SUBSYSTEMS) {
			printk(KERN_INFO "Lophilo ended detection, maximum found %d\n", MAX_SUBSYSTEMS);
			break;
		}

		subsystems[subsystem_id].addr = current_addr;
		subsystems[subsystem_id].id = ioread32(current_addr + 0x4);

		if((subsystems[subsystem_id].id & 0xea000000) == 0xea000000) {
			printk(KERN_INFO "Lophilo adding subsystem 0x%x of type 0x%x\n", subsystem_id, subsystems[subsystem_id].id);
		} else {
			printk(KERN_INFO "Lophilo ended detection, found 0x%x\n", subsystems[subsystem_id].id);
			break;
		}


		subsystems[subsystem_id].size = ioread32(current_addr);
		//printk(KERN_INFO "Subsystem size 0x%x\n", subsystems[subsystem_id].size);

		sprintf(subsystem_str, "subsystem%d", subsystem_id);
		debugfs_create_dir(
			subsystem_str,
			lophilo_dentry);

		debugfs_create_x32(
			"size",
			S_IRWXU | S_IRWXG | S_IRWXO,
			lophilo_subsystem_dentry,
			&subsystems[subsystem_id].size);

		debugfs_create_x32(
			"id",
			S_IRWXU | S_IRWXG | S_IRWXO,
			lophilo_subsystem_dentry,
			&subsystems[subsystem_id].id);

		debugfs_create_x32(
			"addr",
			S_IRWXU | S_IRWXG | S_IRWXO,
			lophilo_subsystem_dentry,
			&subsystems[subsystem_id].addr);

		debugfs_create_file(
			"mem",
			S_IRWXU | S_IRWXG | S_IRWXO,
			lophilo_subsystem_dentry,
			&subsystems[subsystem_id],
			&fops
			);
		current_addr += subsystems[subsystem_id].size;
		//printk(KERN_INFO "current_addr increment to 0x%x for subsystem 0x%x", current_addr, subsystem_id);
		subsystem_id++;

	}

	return 0;
}

void __exit
lophilo_cleanup(void)
{
	printk(KERN_INFO "Lophilo module uninstalling\n");
	debugfs_remove_recursive(lophilo_dentry);
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
