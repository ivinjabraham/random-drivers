#include <linux/init.h>
#include <linux/module.h>
MODULE_LICENSE("GPL");

static int __init hello_init(void) 
{
    printk(KERN_ALERT "hello, world");
    return 0;
}

static void hello_exit(void)
{
    printk(KERN_ALERT "Goodbye");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_AUTHOR("Ivin");
MODULE_DESCRIPTION("Prints hello and goodbye");
