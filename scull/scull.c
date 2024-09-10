// ?
#include <linux/stat.h>
#include <linux/sysctl.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/fs.h>

#include <linux/errno.h> // std. error definitions
#include <asm/current.h> // For `current` struct
#include <linux/slab.h> // malloc and related functions
#include <linux/kern_levels.h> // FMT definitions for printk
#include <linux/container_of.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>

#define BUFFER_SIZE 1000 // TODO: Dynamic buffer
#define NUMBER_OF_DEVICE_NUMBERS_ASSOCIATED_WITH_DEVICE 1
// IOCTL
#define SCULL_IOC_MAGIC '1'
#define SCULL_IOC_RESET _IO(SCULL_IOC_MAGIC, 1)
#define SCULL_IOC_MAXNR 1
MODULE_LICENSE("Dual BSD/GPL");

// TODO: Concurrency with binary sem?
typedef struct scull_dev {
	char *device_buffer; // "Device"
	struct cdev cdev;
} scull_dev;
scull_dev *scull_devices;

static dev_t device_num;
static int num_devices = 1;
static int major_num = 0;
static int minor_num = 0;
module_param(num_devices, int, S_IRUGO);
module_param(major_num, int, S_IRUGO);
module_param(minor_num, int, S_IRUGO);

static int scull_open(struct inode *inode, struct file *flip)
{
	// TODO: Crash when calling current?
	printk(KERN_DEBUG "Opening scull.");
	scull_dev *dev = container_of(inode->i_cdev, scull_dev, cdev);
	flip->private_data = dev;

	return 0;
}

static ssize_t scull_read(struct file *filp, char *__user buf, size_t len,
			  loff_t *f_pos)
{
	printk(KERN_DEBUG "Reading from device.\n");
	ssize_t bytes_read;

	// EOF
	if (*f_pos >= BUFFER_SIZE) {
		return 0;
	}

	// Handle out of bounds read
	if (len > BUFFER_SIZE - *f_pos) {
		len = BUFFER_SIZE - *f_pos;
	}

	scull_dev *dev = filp->private_data;
	if (copy_to_user(buf, dev->device_buffer + *f_pos, len)) {
		return -EFAULT;
	}
	*f_pos += len;

	bytes_read = len;
	printk(KERN_DEBUG "Successfully read %zu bytes from device",
	       bytes_read);
	return bytes_read;
}

static int scull_release(struct inode *inode, struct file *filp)
{
	printk(KERN_DEBUG "Scull released.");
	return 0;
}

static ssize_t scull_write(struct file *filp, const char *__user buf,
			   size_t len, loff_t *f_pos)
{
	printk(KERN_DEBUG "Writing to device.");
	ssize_t bytes_written;

	// EOF
	if (*f_pos >= BUFFER_SIZE) {
		return 0;
	}

	// Handle out of bounds write
	if (len > BUFFER_SIZE - *f_pos) {
		len = BUFFER_SIZE - *f_pos;
	}

	scull_dev *dev = filp->private_data;
	if (copy_from_user(dev->device_buffer + *f_pos, buf, len)) {
		return -EFAULT;
	}

	*f_pos += len;

	bytes_written = len;
	printk(KERN_DEBUG "Successfully wrote %zu bytes to device.",
	       bytes_written);
	return bytes_written;
}

static long scull_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	scull_dev *dev = filp->private_data;
	if (_IOC_TYPE(cmd) != SCULL_IOC_MAGIC)
		return -ENOTTY;
	if (_IOC_NR(cmd) > SCULL_IOC_MAXNR)
		return -ENOTTY;

	switch (cmd) {
	case SCULL_IOC_RESET:
		memset(dev->device_buffer, 0, BUFFER_SIZE);
		printk(KERN_DEBUG "Device buffer reset.\n");
		break;

	default:
		return -ENOTTY;
	}

	return 0;
}
static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = scull_open,
	.read = scull_read,
	.release = scull_release,
	.write = scull_write,
	.unlocked_ioctl = scull_ioctl,
};

static int reg_scull(void)
{
	int result;

	if (major_num) {
		printk(KERN_DEBUG "Registering chrdev region statically.\n");
		device_num = MKDEV(major_num, minor_num);
		result = register_chrdev_region(device_num, num_devices,
						"scull");
	} else {
		printk(KERN_DEBUG "Registering chrdev region dynamically.\n");
		result = alloc_chrdev_region(&device_num, 0, num_devices,
					     "scull");
	}

	return result;
}

static int cdev_setup(scull_dev *devp, int index)
{
	printk(KERN_DEBUG
	       "Creating CDEV for device %i with major number %u and minor number %u",
	       index, MAJOR(device_num), MINOR(device_num) + index);

	int result;
	dev_t dev_no = MKDEV(MAJOR(device_num), MINOR(device_num) + index);

	cdev_init(&devp->cdev, &fops);
	devp->cdev.owner = THIS_MODULE;
	devp->cdev.ops = &fops;

	result = cdev_add(&devp->cdev, dev_no,
			  NUMBER_OF_DEVICE_NUMBERS_ASSOCIATED_WITH_DEVICE);

	return result;
}

static int __init scull_init(void)
{
	printk(KERN_DEBUG "Loading SCULL.\n");
	int result;

	printk(KERN_DEBUG "Registering device number.\n");

	result = reg_scull();
	if (result < 0) {
		printk(KERN_ERR "Failed to register device number.\n");
		return result;
	}

	printk(KERN_DEBUG
	       "Registration succeeded major number %u and minor number starting from %u \
            for %u number of devices.\n",
	       MAJOR(device_num), MINOR(device_num), num_devices);

	printk(KERN_DEBUG "Allocating devices.\n");

	scull_devices = kzalloc(num_devices * sizeof(scull_dev), GFP_KERNEL);
	if (!scull_devices) {
		printk(KERN_ERR "Could not allocate memory for devices.\n");
		result = -ENOMEM;
		goto err_chrdev;
	}

	printk(KERN_DEBUG "Successfully allocated memory for devices.\n");

	printk(KERN_DEBUG "Allocating buffers for devices.\n");
	for (int i = 0; i < num_devices; i++) {
		scull_devices[i].device_buffer =
			kzalloc(BUFFER_SIZE, GFP_KERNEL);
		if (!scull_devices[i].device_buffer) {
			printk(KERN_ERR
			       "Failed to allocate buffer for device %d",
			       i);
			result = -ENOMEM;

			printk(KERN_DEBUG
			       "Freeing memory for the %d previously allocated buffer(s).\n",
			       i);
			int j;
			for (j = 0; j < i; j++) {
				kfree(scull_devices[j].device_buffer);
			}
			printk(KERN_DEBUG "%d buffers freed.\n", j);

			goto err_cdev_setup;
		}
	}
	printk(KERN_DEBUG "Successfully allocated buffers for devices.\n");

	for (int i = 0; i < num_devices; i++) {
		result = cdev_setup(&scull_devices[i], i);
		if (result) {
			printk(KERN_ERR "Failed to setup cdev.");
			// TODO: Repeated twice, might be useful to abstract into fn.
			printk(KERN_DEBUG
			       "Deleting previously added %d CDEV(s).\n",
			       i);
			int j;
			for (j = 0; j < i; j++) {
				cdev_del(&scull_devices[j].cdev);
			}
			printk(KERN_DEBUG "Deleted %d CDEV(s).\n", j);
			goto err_cdev_setup;
		}
	}

	printk(KERN_DEBUG "Ready for use");
	printk(KERN_INFO "Ready.");

	return 0; // TODO: two returns?

err_cdev_setup:
	kfree(scull_devices);
err_chrdev:
	unregister_chrdev_region(device_num, num_devices);

	return 0; // TODO: return last err or success on exit?
}

static void __exit scull_exit(void)
{
	for (int i = 0; i < num_devices; i++) {
		cdev_del(&scull_devices[i].cdev);
	}
	printk(KERN_DEBUG "Deleted cdev(s).\n");

	for (int i = 0; i < num_devices; i++) {
		kfree(scull_devices[i].device_buffer);
	}
	kfree(scull_devices);
	printk(KERN_DEBUG "Deleted buffers.\n");

	unregister_chrdev_region(device_num, num_devices);

	printk(KERN_DEBUG "Unregistered device numbers.\n");
	printk(KERN_DEBUG "Unloading SCULL. Goodbye.\n");
}

module_init(scull_init);
module_exit(scull_exit);
