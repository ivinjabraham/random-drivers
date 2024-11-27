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

#define BUFFER_SIZE 8000 // TODO: Dynamic buffer
#define NUMBER_OF_DEVICE_NUMBERS_ASSOCIATED_WITH_DEVICE 1
// IOCTL
#define SCULL_IOC_MAGIC '1'
#define SCULL_IOC_RESET _IO(SCULL_IOC_MAGIC, 1)
#define SCULL_IOC_MAXNR 1

#ifdef SCULL_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "scull: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

static int num_devices = 1;
module_param(num_devices, int, S_IRUGO);

// TODO: Concurrency with binary sem?
typedef struct scull_dev {
	char *device_buffer; // "Device"
	struct cdev cdev;
} scull_dev;

scull_dev *scull_devices;
static dev_t device_num;

