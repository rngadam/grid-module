/*
 * Demonstration of the use of debugfs with Lophilo
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
#include <linux/mm.h> // remap_pfn_range
#include <asm/page.h> // page_to_pfn

// from linux/arch/arm/mach-at91/board-tabby.c
extern void __iomem *fpga_cs0_base;
extern void __iomem *fpga_cs1_base;
extern void __iomem *fpga_cs2_base;
extern void __iomem *fpga_cs3_base;

#define MAX_SUBSYSTEMS 32

#define GPIO_SUBSYSTEM 0xea680001
#define PWM_SUBSYSTEM 0xea680002

#define SYS_PHYS_ADDR 0x10000000
#define MOD_PHYS_ADDR 0x20000000

struct subsystem {
	u32 id;
	u32 size;
	u32 vaddr;
	u32 offset;
	u32 index;
	u32 paddr;
	u8 opened;
};

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);
static int map_lophilo(struct file *filp, struct vm_area_struct *vma);

struct file_operations fops_mem = {
	.owner   = THIS_MODULE,
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release,
	.mmap    = map_lophilo
 };

static struct subsystem subsystems[MAX_SUBSYSTEMS];

static struct subsystem sys_subsystem = {
	.id = 0,
	.size = 0x204,
	.paddr = SYS_PHYS_ADDR,
	.offset = 0
};

static struct subsystem mod_subsystem = {
	.id = 1,
	.paddr = MOD_PHYS_ADDR,
	.offset = 0
};

char registry[16000];
static struct debugfs_blob_wrapper registry_blob = {
	.data = registry,
	.size = 0
};

struct resource * fpga;

static struct dentry *lophilo_dentry;

char parent_name[64]; // for generating names

#define CREATE_CHANNEL_FILE(size, name, offset) \
	debugfs_create_x##size( \
		name, \
		S_IRWXU | S_IRWXG | S_IRWXO, \
		root, \
		(void*) addr + offset); \
	create_registry_entry(size, parent_name, name, addr, offset);

void create_registry_entry(u8 size, char* parent_name, char* name, u32 addr, u32 offset)
{
	char* type_sys = "sys";
	char* type_mod = "mod";
	char* type;
	if(addr < (u32)fpga_cs1_base) {
		type = type_sys;
		offset += addr - (u32)fpga_cs0_base;
	}  else {
		type = type_mod;
		offset += addr - (u32)fpga_cs1_base;
	}
	sprintf(&registry[registry_blob.size],
		"%s %u %s %s %u\n",
		type, size, parent_name, name, offset);
	printk(KERN_INFO "%s", &registry[registry_blob.size]);
	registry_blob.size += strlen(&registry[registry_blob.size]);
}


 struct dentry * create_channel_gpio(u8 id, struct dentry * parent, u32 addr)
 {
 	int i;
 	struct dentry * root;
 	char io_name[3];

 	sprintf(parent_name, "gpio%d", id);
 	root  = debugfs_create_dir(parent_name, parent);

 	CREATE_CHANNEL_FILE(32, "dout", 0x8);
 	CREATE_CHANNEL_FILE(32, "din", 0xc);
 	CREATE_CHANNEL_FILE(32, "doe", 0x10);
 	CREATE_CHANNEL_FILE(32, "imask", 0x20);
 	CREATE_CHANNEL_FILE(32, "iclr", 0x24);
 	CREATE_CHANNEL_FILE(32, "ie", 0x28);
 	CREATE_CHANNEL_FILE(32, "iinv", 0x2c);
 	CREATE_CHANNEL_FILE(32, "iedge", 0x30);
 	for(i=0; i<26; i++) {
 		sprintf(io_name, "io%d", i);
 		CREATE_CHANNEL_FILE(8, io_name, 0x40 + i);
 	}
 	return root;
 }

 struct dentry * create_led(u8 id,  struct dentry * parent, u32 addr)
 {
 	struct dentry * root;

	sprintf(parent_name, "led%d", id);
	root = debugfs_create_dir(parent_name, parent);

 	CREATE_CHANNEL_FILE(8, "b",  0x100 + 0x4 * id);
 	CREATE_CHANNEL_FILE(8, "g",  0x101 + 0x4 * id);
 	CREATE_CHANNEL_FILE(8, "r",  0x102 + 0x4 * id);
 	CREATE_CHANNEL_FILE(8, "s",  0x103 + 0x4 * id);
 	CREATE_CHANNEL_FILE(32, "srgb",  0x100);

 	return root;
 }

 struct dentry * create_channel_pwm(u8 id, struct dentry * parent, u32 addr)
 {
 	struct dentry * root;

 	sprintf(parent_name, "pwm%d", id);
 	root = debugfs_create_dir(parent_name, parent);

 	CREATE_CHANNEL_FILE(8, "reset", 0x8);
 	CREATE_CHANNEL_FILE(8, "outinv", 0x9);
 	CREATE_CHANNEL_FILE(8, "pmen", 0xa);
 	CREATE_CHANNEL_FILE(8, "fmen", 0xb);
 	CREATE_CHANNEL_FILE(32, "gate", 0xc);
 	CREATE_CHANNEL_FILE(32, "dtyc", 0x10);

	return root;
 }

 void create_root(struct dentry * root, u32 addr)
 {
 	strcpy(parent_name, "lophilo");
 	CREATE_CHANNEL_FILE(16, "id", 0x0);
	CREATE_CHANNEL_FILE(16, "flag", 0x2);
	CREATE_CHANNEL_FILE(32, "ver", 0x4);
	CREATE_CHANNEL_FILE(32, "lock", 0x8);
	CREATE_CHANNEL_FILE(32, "lockb", 0xc);
	CREATE_CHANNEL_FILE(32, "power", 0x200);
}

static int __init
lophilo_init(void)
{
	struct dentry *lophilo_subsystem_dentry;
	int subsystem_id = 0;

	void* current_addr;
	int i;
	u8 pwm_id = 0;
	u8 gpio_id = 0;

	printk(KERN_INFO "Lophilo module loading\n");

	lophilo_dentry = debugfs_create_dir(
		"lophilo",
		NULL);
	if(lophilo_dentry == NULL) {
		printk(KERN_ERR "Could not create root directory entry lophilo in debugfs");
		return -EINVAL;
	}
	//fpga = request_mem_region(FPGA_BASE_ADDR, SIZE16MB, "Lophilo FPGA LEDs");
	sys_subsystem.vaddr = (u32) fpga_cs0_base;
	mod_subsystem.vaddr = (u32) fpga_cs1_base;

	create_root(lophilo_dentry, sys_subsystem.vaddr);

	debugfs_create_file(
		"sysmem",
		S_IRWXU | S_IRWXG | S_IRWXO,
		lophilo_dentry,
		&sys_subsystem,
		&fops_mem
		);

	debugfs_create_file(
		"modmem",
		S_IRWXU | S_IRWXG | S_IRWXO,
		lophilo_dentry,
		&mod_subsystem,
		&fops_mem
		);

	for(i=0; i<4; i++) {
		create_led(i, lophilo_dentry, sys_subsystem.vaddr);
	}

	current_addr = fpga_cs1_base;

	while(true) {
		if(subsystem_id == MAX_SUBSYSTEMS) {
			printk(KERN_INFO "Lophilo ended detection, maximum found %d\n",
				MAX_SUBSYSTEMS);
			break;
		}

		subsystems[subsystem_id].id = ioread32(current_addr + 0x4);
		subsystems[subsystem_id].offset = fpga_cs1_base - current_addr;

		if((subsystems[subsystem_id].id & 0xea000000) == 0xea000000) {
			printk(KERN_INFO "Lophilo adding subsystem 0x%x of type 0x%x at 0x%x\n",
				subsystem_id, subsystems[subsystem_id].id, (u32)current_addr);
		} else {
			printk(KERN_INFO "Lophilo ended detection, found 0x%x\n",
				subsystems[subsystem_id].id);
			break;
		}

		subsystems[subsystem_id].vaddr = (u32) current_addr;
		subsystems[subsystem_id].paddr = MOD_PHYS_ADDR;

		subsystems[subsystem_id].size = ioread32(current_addr);

		switch(subsystems[subsystem_id].id) {
			case GPIO_SUBSYSTEM:
				lophilo_subsystem_dentry = create_channel_gpio(
					gpio_id++,
					lophilo_dentry,
					subsystems[subsystem_id].vaddr);
				break;
			case PWM_SUBSYSTEM:
				lophilo_subsystem_dentry = create_channel_pwm(
					pwm_id++,
					lophilo_dentry,
					subsystems[subsystem_id].vaddr);
				break;
			default:
				printk(KERN_ERR "Unsupported file system id %d", subsystems[subsystem_id].id);
				return -EINVAL;

		}

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
			&subsystems[subsystem_id].vaddr);


		current_addr += subsystems[subsystem_id].size;
		mod_subsystem.size += subsystems[subsystem_id].size;
		//printk(KERN_INFO "current_addr increment to 0x%x for subsystem 0x%x", current_addr, subsystem_id);
		subsystem_id++;

	}

	debugfs_create_blob(
		"registry",
		S_IRWXU | S_IRWXG | S_IRWXO,
		lophilo_dentry,
		&registry_blob);
	return 0;
}

static int
map_lophilo(struct file *filp, struct vm_area_struct *vma)
{
	long unsigned int size = vma->vm_end - vma->vm_start;
	long unsigned int target_size = PAGE_SIZE;
	struct subsystem* subsystem_ptr = (struct subsystem*) filp->private_data;


	if(size != target_size) {
		printk(KERN_INFO "Invalid allocation request, expected %ld, got %ld",
			target_size, size);
                return -EAGAIN;
	}

	/*

	Comment adapted from:

	http://fixunix.com/kernel/242682-mapping-pci-memory-user-space.html

	There is no relationship between the address returned from ioremap and
	what you pass into io_remap_page_range(). ioremap gives you a kernel
	virtual address for the hardware address you remap. io_remap_page_range()
	creates a userspace mapping in the same way, and you should pass in
	the hw address exactly the same way you pass in the hw address into
	ioremap. io_remap_pfn_range() takes a PFN ("page frame number"),
	which is basically the hw address you want to map divided by
	PAGE_SIZE. The main reason for using PFNs is that they allow you to
	map addresses above 4G even if sizeof long is only 4.
	*/
	if (remap_pfn_range(
			vma,
			vma->vm_start,
			subsystem_ptr->paddr >> PAGE_SHIFT,
			PAGE_SIZE,
			vma->vm_page_prot)) {
		printk(KERN_INFO "Allocation failed!");
                return -EAGAIN;
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

   if (subsystem_ptr->opened)
   	return -EBUSY;
   subsystem_ptr->opened++;

   subsystem_ptr->index = 0;
   file->private_data = subsystem_ptr;

   return 0;
}


/* Called when a process closes the device file */
static int device_release(struct inode *inode, struct file *file)
{
   struct subsystem* subsystem_ptr = inode->i_private;

   subsystem_ptr->opened --;     /* We're now ready for our next caller */

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

   if(subsystem_ptr->index >= subsystem_ptr->size)
   	return 0;

   /* Actually put the data into the buffer */
   while (length && (subsystem_ptr->index < subsystem_ptr->size))  {

        /* The buffer is in the user data segment, not the kernel segment;
         * assignment won't work.  We have to use put_user which copies data from
         * the kernel data segment to the user data segment. */
         put_user(
         	*((char*) (subsystem_ptr->vaddr + subsystem_ptr->index)),
         	buffer++);
         subsystem_ptr->index++;

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
