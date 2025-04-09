#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/kernel.h>

#define DEVICE_NAME "fourinarow"
#define CLASS_NAME "fourinarow"
#define BUFFER_CAPACITY 113
#define BOARD_HEIGHT 8
#define BOARD_WIDTH 8

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eric Ekey");
MODULE_DESCRIPTION("Connect Four Character Device");

static char board[BOARD_HEIGHT][BOARD_WIDTH];

static dev_t dev_num;
static struct class* CLASS_fourinarow;
static struct device* DEVICE_fourinarow;
static struct cdev character_device;

static char output_buffer[BUFFER_CAPACITY];
static size_t output_buffer_size = 0;

static void board_to_buffer(void) {
    int i;
    memset(output_buffer, '0', BUFFER_CAPACITY);
    output_buffer_size = 0;
    output_buffer_size += snprintf(output_buffer + output_buffer_size, BUFFER_CAPACITY - output_buffer_size, "\n  ABCDEFGH\n  --------\n");


    for (i = BOARD_HEIGHT-1; i >= 0; i--) {
        output_buffer_size += snprintf(output_buffer + output_buffer_size, BUFFER_CAPACITY - output_buffer_size, "%d|%c%c%c%c%c%c%c%c\n", i+1, board[i][0], board[i][1], board[i][2], board[i][3], board[i][4], board[i][5], board[i][6], board[i][7]);
    }
    output_buffer_size += snprintf(output_buffer + output_buffer_size, BUFFER_CAPACITY - output_buffer_size, "\n\n");

}

static ssize_t dev_read(struct file *filep, char *user_buffer, size_t len, loff_t *offset) {
    // For testing
    ssize_t bytes_to_copy;
    if (*offset >= output_buffer_size) {
        return 0;
    }

    bytes_to_copy = min((output_buffer_size - *offset), len);
    copy_to_user(user_buffer, output_buffer + *offset, bytes_to_copy);
    *offset += bytes_to_copy;
    return bytes_to_copy;
}

static ssize_t dev_write(struct file *filep, const char *user_buffer, size_t len, loff_t *offset) {
    return 0;
}

static struct file_operations file_operations = {
    .owner = THIS_MODULE,
    .read = dev_read,
    .write = dev_write,
};

static int __init init_fourinarow(void) {
    printk(KERN_INFO "Loading %s module...\n", DEVICE_NAME);
    memset(board, '0', sizeof(board));
    alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);

    cdev_init(&character_device, &file_operations);
    character_device.owner = THIS_MODULE;
    cdev_add(&character_device, dev_num, 1);

    CLASS_fourinarow = class_create(THIS_MODULE, CLASS_NAME);
    DEVICE_fourinarow = device_create(CLASS_fourinarow, NULL, dev_num, NULL, DEVICE_NAME);


    // for testing
    board_to_buffer();


    printk(KERN_INFO "Module loaded at /dev/%s\n", DEVICE_NAME);
    return 0;
}

static void __exit exit_fourinarow(void) {
    printk(KERN_INFO "Unloading %s module...\n", DEVICE_NAME);
    device_destroy(CLASS_fourinarow, dev_num);
    class_destroy(CLASS_fourinarow);
    cdev_del(&character_device);
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_INFO "Module unloaded from /dev/%s\n", DEVICE_NAME);
}

module_init(init_fourinarow);
module_exit(exit_fourinarow);
