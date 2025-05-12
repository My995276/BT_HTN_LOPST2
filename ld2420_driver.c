#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/serial.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/uaccess.h>

#define DRIVER_NAME "hlk_ld2420_driver"
#define DEVICE_NAME "hlk_ld2420"
#define CLASS_NAME "hlk_ld2420_class"

// UART Configuration for HLK-LD2420
#define HLK_LD2420_UART_BAUDRATE 9600

static struct class *hlk_ld2420_class = NULL;
static struct device *hlk_ld2420_device = NULL;
static struct tty_driver *hlk_ld2420_tty_driver = NULL;

// Buffer to hold radar data
#define RADAR_BUFFER_SIZE 256
static char radar_buffer[RADAR_BUFFER_SIZE];
static int radar_buffer_len = 0;

// File operations
static int hlk_ld2420_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO DRIVER_NAME ": Device opened\n");
    return 0;
}

static int hlk_ld2420_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO DRIVER_NAME ": Device closed\n");
    return 0;
}

static ssize_t hlk_ld2420_read(struct file *file, char __user *buffer, size_t len, loff_t *offset)
{
    int ret;

    if (*offset >= radar_buffer_len)
        return 0;

    if (len > radar_buffer_len - *offset)
        len = radar_buffer_len - *offset;

    ret = copy_to_user(buffer, radar_buffer + *offset, len);
    if (ret != 0)
        return -EFAULT;

    *offset += len;
    printk(KERN_INFO DRIVER_NAME ": Data read from radar buffer\n");
    return len;
}

static ssize_t hlk_ld2420_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset)
{
    char command[128] = {0};

    if (len > sizeof(command) - 1)
        len = sizeof(command) - 1;

    if (copy_from_user(command, buffer, len))
        return -EFAULT;

    command[len] = '\0';
    printk(KERN_INFO DRIVER_NAME ": Command sent to radar: %s\n", command);

    // TODO: Send this command to the radar via UART
    return len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = hlk_ld2420_open,
    .release = hlk_ld2420_release,
    .read = hlk_ld2420_read,
    .write = hlk_ld2420_write,
};

// Module initialization
static int __init hlk_ld2420_init(void)
{
    int major_number;
    printk(KERN_INFO DRIVER_NAME ": Initializing HLK-LD2420 driver\n");

    // Allocate major number
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ALERT DRIVER_NAME ": Failed to register major number\n");
        return major_number;
    }
    printk(KERN_INFO DRIVER_NAME ": Registered with major number %d\n", major_number);

    // Create device class
    hlk_ld2420_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(hlk_ld2420_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT DRIVER_NAME ": Failed to register device class\n");
        return PTR_ERR(hlk_ld2420_class);
    }

    // Create device
    hlk_ld2420_device = device_create(hlk_ld2420_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
if (IS_ERR(hlk_ld2420_device)) {
        class_destroy(hlk_ld2420_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT DRIVER_NAME ": Failed to create device\n");
        return PTR_ERR(hlk_ld2420_device);
    }

    printk(KERN_INFO DRIVER_NAME ": Driver initialized successfully\n");
    return 0;
}

// Module cleanup
static void __exit hlk_ld2420_exit(void)
{
    device_destroy(hlk_ld2420_class, MKDEV(0, 0));
    class_destroy(hlk_ld2420_class);
    unregister_chrdev(0, DEVICE_NAME);
    printk(KERN_INFO DRIVER_NAME ": Driver exited\n");
}

module_init(hlk_ld2420_init);
module_exit(hlk_ld2420_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Linux Kernel Driver for HLK-LD2420 Radar Module");
MODULE_VERSION("1.0");
