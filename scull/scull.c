#include "linux/device.h"
#include "linux/fs.h"
#include "linux/kern_levels.h"
#include "linux/minmax.h"
#include "linux/moduleparam.h"
#include "linux/printk.h"
#include "linux/slab.h"
#include "linux/stat.h"
#include "linux/sysctl.h"
#include "linux/uaccess.h"
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>

#define BUFFER_SIZE 1000

#define SCULL_IOC_MAGIC 'k'
#define SCULL_IOC_RESET _IO(SCULL_IOC_MAGIC, 1)
#define SCULL_IOC_MAXNR 1

MODULE_LICENSE("GPL");

static dev_t device_num;
static int init_res;
static int num_devices = 1;
static int major_num = 0;
static int minor_num = 0;
static char* device_buffer;

// module_param(num_devices, int, S_IRUGO);
module_param(major_num, int, S_IRUGO);
module_param(minor_num, int, S_IRUGO);

static int open(struct inode *inode, struct file *flip) 
{
    printk(KERN_DEBUG "Scull opened");
    return 0;
}

static ssize_t read(struct file* filp, char* __user buf, size_t len, loff_t* f_pos) 
{
    printk(KERN_DEBUG "Reading from device");
    ssize_t bytes_read;

    if (*f_pos >= BUFFER_SIZE) {
        return 0;
    }

    if (len > BUFFER_SIZE - *f_pos) {
        len = BUFFER_SIZE - *f_pos;
    }

    if (copy_to_user(buf, device_buffer + *f_pos, len)) {
        return -EFAULT;
    }

    *f_pos += len;
    bytes_read = len;

    printk(KERN_DEBUG "Successfully read %zu bytes from device", bytes_read);
    return bytes_read;
}

static int release(struct inode* inode, struct file* filp) 
{
    printk(KERN_DEBUG "Scull released.");
    return 0;
}

static ssize_t my_write(struct file* filp, const char* __user buf, size_t len, loff_t* f_pos) 
{
    ssize_t bytes_written;

    printk(KERN_DEBUG "Writing to device.");
    if (*f_pos >= BUFFER_SIZE) {
        return 0;
    }

    if (len > BUFFER_SIZE - *f_pos) {
        len = BUFFER_SIZE - *f_pos;
    }

    if (copy_from_user(device_buffer + *f_pos, buf, len)) {
        return -EFAULT;
    }

    *f_pos += len;
    bytes_written = len;

    printk(KERN_DEBUG "Successfully wrote %zu bytes to device.", bytes_written);
    return bytes_written;
}

static long ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    char *buffer;
    size_t len;

    switch(cmd) {
        case SCULL_IOC_RESET:
            memset(device_buffer, 0, BUFFER_SIZE);
            printk(KERN_DEBUG "Device buffer reset");
            break;

        default:
            return -ENOTTY;
    }

    return 0;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = open,
    .read = read,
    .release = release,
    .write = my_write, 
    .unlocked_ioctl = ioctl,
};


static int reg_scull(void) 
{
    printk(KERN_DEBUG "Registering device numbers...");
    int reg_res;

    if (major_num) {
        device_num = MKDEV(major_num, minor_num);
        printk(KERN_DEBUG "register, not allocating chrdev");
        reg_res = register_chrdev_region(device_num, num_devices, "scull");
    } else {
        printk(KERN_DEBUG "allocate, not register chrdev");
        reg_res = alloc_chrdev_region(&device_num, 0, num_devices, "scull");
    }

    major_num = MAJOR(device_num);
    minor_num = MINOR(device_num);
    printk(KERN_DEBUG "major number: %u minor: %u", major_num, minor_num);

    return reg_res;
}

struct scull_dev {
    struct cdev cdev;
};

struct scull_dev* scull_devp;

static int cdev_setup(struct scull_dev* devp, int index) 
{
    int err;
    printk(KERN_DEBUG "creating cdev with mjr %u mnr %u", major_num, minor_num);
    int device_num = MKDEV(major_num, minor_num + index); 

    cdev_init(&devp->cdev, &fops);
    devp->cdev.owner = THIS_MODULE;
    devp->cdev.ops = &fops;

    err = cdev_add(&devp->cdev, device_num, 1);
    printk(KERN_DEBUG "SHOULD BE FINE RIGHT err: %d", err);

    return err;
}

struct scull_dev scull_dev;

static int __init init(void)
{

    device_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!device_buffer) {
        printk(KERN_ERR "Failed to allocate device buffer");
        return -ENOMEM;
    }

    init_res = reg_scull();
    if (init_res < 0) {
        printk(KERN_ERR "Failed to register device number."); 
        return init_res;
    } else {
        printk(KERN_DEBUG "Registration succeeded major number %u and minor number starting from %u \
                for %u number of devices.", major_num, minor_num, num_devices);
    }

    int cdev_err;
    cdev_err = cdev_setup(&scull_dev, 0);
    if (cdev_err) {
        printk(KERN_ERR "Failed to setup cdev.");
        return cdev_err;
    }

    printk(KERN_DEBUG "Ready for use");
    return 0;
}

static void __exit exit(void)
{
    unregister_chrdev_region(device_num, num_devices);
    printk(KERN_DEBUG "Unregistered device numbers.\n");

    cdev_del(&scull_dev.cdev);
    printk(KERN_DEBUG "Deleted cdev.");

    kfree(device_buffer);
    printk(KERN_DEBUG "Freed buffer");

    printk(KERN_DEBUG "Unloading scull.");
}

module_init(init);
module_exit(exit);
