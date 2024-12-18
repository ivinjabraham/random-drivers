//  SCULL Driver: based off of Linux Device Drivers edtn. 3
//  Copyright (C) 2024 Ivin Joel Abraham

//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.

//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.

//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <linux/stat.h>
#include <linux/sysctl.h>
#include <linux/uaccess.h>
#include <linux/device.h>

#include <linux/fs.h> // 'file' struct
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

#include "scull.h"

MODULE_LICENSE("GPL");


static int scull_open(struct inode *inode, struct file *filp)
{
	scull_dev *dev = container_of(inode->i_cdev, scull_dev, cdev);
	filp->private_data = dev;

	return 0;
}

static ssize_t scull_read(struct file *filp, char *__user buf, size_t len,
			  loff_t *f_pos)
{
	ssize_t bytes_read;

	// EOF
	if (*f_pos >= buffer_size) {
		return 0;
	}

	// Handle out of bounds read
	if (len > buffer_size - *f_pos) {
		len = buffer_size - *f_pos;
	}

	scull_dev *dev = filp->private_data;
	if (copy_to_user(buf, dev->device_buffer + *f_pos, len)) {
		return -EFAULT;
	}

	*f_pos += len;

	bytes_read = len;
	return bytes_read;
}

static int scull_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t scull_write(struct file *filp, const char *__user buf,
			   size_t len, loff_t *f_pos)
{
	ssize_t bytes_written;

	// EOF
	if (*f_pos >= buffer_size) {
		return 0;
	}

	// Handle out of bounds write
	if (len > buffer_size - *f_pos) {
		len = buffer_size - *f_pos;
	}

	scull_dev *dev = filp->private_data;
	if (copy_from_user(dev->device_buffer + *f_pos, buf, len)) {
		return -EFAULT;
	}

	*f_pos += len;

	bytes_written = len;
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
		memset(dev->device_buffer, 0, buffer_size);
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

        result = alloc_chrdev_region(&device_num, 0, num_devices,
                "scull");

	return result;
}

static int cdev_setup(scull_dev *devp, int index)
{
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
	int result;

	result = reg_scull();
	if (result < 0) {
		PDEBUG("scull: Failed to register device number.\n");
		return result;
	}

	PDEBUG("scull: Registration succeeded for major number %u and minor number startings from %u for %u number of devices.\n",
	       MAJOR(device_num), MINOR(device_num), num_devices);

	scull_devices = kzalloc(num_devices * sizeof(scull_dev), GFP_KERNEL);
	if (!scull_devices) {
		PDBEUG("scull: Could not allocate memory for devices.\n");
		result = -ENOMEM;
		goto err_chrdev;
	}

	for (int i = 0; i < num_devices; i++) {
		scull_devices[i].device_buffer =
			kzalloc(buffer_size, GFP_KERNEL);
		if (!scull_devices[i].device_buffer) {
			PDBEUG("scull: Failed to allocate buffer for device %d", i);
			result = -ENOMEM;

			int j;
			for (j = 0; j < i; j++) {
				kfree(scull_devices[j].device_buffer);
			}
			goto err_cdev_setup;
		}
	}

	for (int i = 0; i < num_devices; i++) {
		result = cdev_setup(&scull_devices[i], i);
		if (result) {
			PDEBUG("scull: Failed to setup cdev.");
			int j;
			for (j = 0; j < i; j++) {
				cdev_del(&scull_devices[j].cdev);
			}
			goto err_cdev_setup;
		}
	}

	printk(KERN_INFO "SCULL: Ready for use.\n");

	return 0;

err_cdev_setup:
	kfree(scull_devices);
err_chrdev:
	unregister_chrdev_region(device_num, num_devices);

	printk(KERN_ERR "SCULL: Failed to initialize.\n");
	return result;
}

static void __exit scull_exit(void)
{
	for (int i = 0; i < num_devices; i++) {
		cdev_del(&scull_devices[i].cdev);
	}

	for (int i = 0; i < num_devices; i++) {
		kfree(scull_devices[i].device_buffer);
	}
	kfree(scull_devices);

	unregister_chrdev_region(device_num, num_devices);

	printk(KERN_INFO "SCULL: Unloading SCULL. Goodbye.\n");
}

module_init(scull_init);
module_exit(scull_exit);
