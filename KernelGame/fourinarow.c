#include "fourinarow.h"

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/kernel.h>

#define DEVICE_NAME "fourinarow"
#define CLASS_NAME "connectfour"
#define BUFFER_CAPACITY 1024

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eric Ekey");
MODULE_DESCRIPTION("Connect Four Character Device");

static int major_num;
static struct class* connectfour = NULL;
static struct device* connectfour_device = NULL;
static struct cdev character_device;

static char write_buffer[BUFFER_CAPACITY] = {0};
static size_t write_buffer_size = 0;

static ssize_t dev_read(struct file *filep, char *user_buffer, size_t len, loff_t *offset) {
    return 0;
}

static ssize_t dev_write(struct file *filep, const char *user_buffer, size_t len, loff_t *offset) {
    if (copy_from_user(write_buffer, user_buffer, len) != 0) {
        return -EFAULT;
    }
    write_buffer[len] = '\0';
    write_buffer_size = len;
    
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = dev_read,
    .write = dev_write,
};

static int __init four_init(void) {
    printk(KERN_INFO "fourinarow: Initializing\n");

    major_num = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_num < 0) {
        printk(KERN_ALERT "fourinarow: Failed to register a major number\n");
        return major_num;
    }

    connectfour = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(connectfour)) {
        unregister_chrdev(major_num, DEVICE_NAME);
        return PTR_ERR(connectfour);
    }

    connectfour_device = device_create(connectfour, NULL, MKDEV(major_num, 0), NULL, DEVICE_NAME);
    if (IS_ERR(connectfour_device)) {
        class_destroy(connectfour);
        unregister_chrdev(major_num, DEVICE_NAME);
        return PTR_ERR(connectfour_device);
    }

    printk(KERN_INFO "fourinarow: Device created at /dev/%s\n", DEVICE_NAME);
    return 0;
}

static void __exit four_exit(void) {
    device_destroy(connectfour, MKDEV(major_num, 0));
    class_unregister(connectfour);
    class_destroy(connectfour);
    unregister_chrdev(major_num, DEVICE_NAME);
    printk(KERN_INFO "fourinarow: Module unloaded\n");
}

module_init(four_init);
module_exit(four_exit);