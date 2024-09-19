//  Dead simple driver that simply greets on load and unload. 
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
