#include "linux/bitmap.h"
#include "linux/fs.h"
#include "linux/kdev_t.h"
#include "linux/kern_levels.h"
#include "linux/moduleparam.h"
#include "linux/stat.h"
#include <linux/init.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
MODULE_LICENSE("GPL");

static dev_t device_num;
static int init_res;
static int num_devices = 1;
static int major_num = 0;
static int minor_num = 0;

module_param(num_devices, int, S_IRUGO);
module_param(major_num, int, S_IRUGO);
module_param(minor_num, int, S_IRUGO);

static int __init init(void)
{
    printk(KERN_DEBUG "Registering device numbers...");

    int reg_res;

    if (major_num) {
        device_num = MKDEV(major_num, minor_num);
        reg_res = register_chrdev_region(device_num, num_devices, "scull");
    } else {
        reg_res = alloc_chrdev_region(&device_num, 0, num_devices, "scull");
    }

    if (reg_res < 0) {
        printk(KERN_ERR "Failed to register device number."); 
        init_res = reg_res;
        return init_res;
    } else {
        printk(KERN_DEBUG "Registration succeeded major number %d and minor number starting from %d \
                for %d number of devices.", major_num, minor_num, num_devices);
        init_res = 0;
    }

    return init_res;
}

static void __exit exit(void)
{
    if (init_res == 0) {
        unregister_chrdev_region(device_num, num_devices);
        printk(KERN_DEBUG "Unregistered device numbers.\n");
    }

    printk(KERN_DEBUG "Unloading scull.");
}

module_init(init);
module_exit(exit);
