/*
 * JingleBellProc.c
 *
 *  Created on: Oct 31, 2013
 *      Author: root
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <asm/io.h>
#include <linux/timer.h>

#define BELL_FILE_NAME "Bell"

struct proc_dir_entry *BellProcFile;
static struct timer_list m_timer;

#define MODE_ADDR         0x10000060
#define IO_DIR_ADDR       0x10000624
#define IO_DATA_ADDR      0x10000620
#define BUTTON_IO_NUM     17
//#define LED_IO_NUM        12
#define LED_IO_NUM        14
#define INFRARED_IO_NUM   14

#define GPIO_NUM(x)       (1 << x)

typedef struct Bell_t {
	char buf[6];
}Bell_t;

static Bell_t bell_t;

static void BellInitGpioMode() {
	unsigned int reg_value;

//	reg_value = inl(MODE_ADDR);
//	reg_value |= (0x1<<2);
//	reg_value |= (0x1<<3);
//	reg_value |= (0x1<<4);
//	reg_value |= (0x1<<6);
//	outl(reg_value, 0x10000060);

	reg_value = inl(IO_DIR_ADDR);

//	reg_value |= (0x1<<20);
//	reg_value &= ~(GPIO_NUM(INFRARED_IO_NUM));
	reg_value &= ~(GPIO_NUM(BUTTON_IO_NUM));
	reg_value |= (GPIO_NUM(LED_IO_NUM));
//	reg_value |= (0x1<<10);

	outl(reg_value, IO_DIR_ADDR);
	printk(KERN_INFO "BellInitGpioMode ok\n");
}

#define OFF 0
#define ON  1
#define RD  0
#define SET 1

static ssize_t BellRead(struct file *filp, char *buffer, size_t length, loff_t * offset) {
//	printk("%d, %d, %d, %d\n", buffer[0], buffer[1], buffer[2], buffer[3]);
	unsigned int reg_value;
	if(SET == buffer[0]) {
		reg_value = inl(IO_DATA_ADDR);
		if(ON != buffer[3]) {
			reg_value |= GPIO_NUM(LED_IO_NUM);
		}
		else {
			reg_value &= ~(GPIO_NUM(LED_IO_NUM));
		}
		outl(reg_value, IO_DATA_ADDR);
	}
	else {
		reg_value = inl(IO_DATA_ADDR);
		buffer[1] = (reg_value & GPIO_NUM(BUTTON_IO_NUM)) ? 0:1; //low when press
//		buffer[2] = (reg_value & GPIO_NUM(INFRARED_IO_NUM)) ? 1:0;
		buffer[2] = 0;
	}
	return 0;
}

static void BellLedctl(int opt) {
	unsigned int reg_value;

	if(0 == opt) {
		reg_value = inl(IO_DATA_ADDR);
		reg_value &=~(GPIO_NUM(12));
		outl(reg_value, IO_DATA_ADDR);
	}
	else {
		reg_value = inl(IO_DATA_ADDR);
		reg_value |= (GPIO_NUM(12));
		outl(reg_value, IO_DATA_ADDR);
	}
}

static void timer_func(unsigned long args) {
	init_timer(&m_timer);
	m_timer.expires = jiffies + HZ;
	add_timer(&m_timer);
	unsigned int reg_value;
	reg_value = inl(0x10000620);
		//	sprintf(buffer, "HelloWorld! %d\n", reg_value);
		//	printk(KERN_INFO "procfile_read (/proc/%d) called\n", reg_value);
	//printk(KERN_INFO "procfile_read (/proc/ %d %d %d) called\n", reg_value & (0x1<<20), reg_value & (0x1<<17), reg_value & (0x1<<14));
}

static int BellPermission(struct inode *inode, int op, struct nameidata *foo)
{
	if(op == 4 || op == 2) {
		return 0;
	}
	return -EACCES;
}

int BellOpen(struct inode *inode, struct file *file)
{
	try_module_get(THIS_MODULE);
	return 0;
}

int BellClose(struct inode *inode, struct file *file)
{
	module_put(THIS_MODULE);
	return 0;
}

static struct file_operations BellFileOps4OurProcFile = {
        .read    = BellRead,
        .open    = BellOpen,
        .release = BellClose,
};

static struct inode_operations BellInodeOps4OurProcFile = {
		.permission = BellPermission,
};

int BellInit()
{
	BellProcFile = create_proc_entry(BELL_FILE_NAME, 0644, NULL);
	if (BellProcFile == NULL) {
		remove_proc_entry(BELL_FILE_NAME, &proc_root);
		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n", BELL_FILE_NAME);
		return -ENOMEM;
	}
	BellProcFile->owner = THIS_MODULE;
	BellProcFile->proc_iops = &BellInodeOps4OurProcFile;
	BellProcFile->proc_fops = &BellFileOps4OurProcFile;
	BellProcFile->mode = S_IFREG | S_IRUGO | S_IWUSR;
	BellProcFile->uid = 0;
	BellProcFile->gid = 0;
	BellProcFile->size = 80;
	BellInitGpioMode();
	return 0;
}
void BellExit()
{
	remove_proc_entry(BELL_FILE_NAME, &proc_root);
	printk(KERN_INFO "/proc/%s removed\n", BELL_FILE_NAME);
}

module_init(BellInit);
module_exit(BellExit);
