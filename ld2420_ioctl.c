#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DRIVER_NAME "hlk_ld2420_driver"
#define CLASS_NAME "hlk_ld2420"
#define DEVICE_NAME "hlk_ld2420"

// Define HLK-LD2420 registers (assume this is I2C-based, replace if needed)
#define HLK_LD2420_REG_DATA 0x00 // Replace with the actual data register address
#define HLK_LD2420_REG_CONFIG 0x01 // Replace with the actual config register address

// IOCTL commands
#define HLK_LD2420_IOCTL_MAGIC 'h'
#define HLK_LD2420_IOCTL_READ_DATA _IOR(HLK_LD2420_IOCTL_MAGIC, 1, int)
#define HLK_LD2420_IOCTL_SET_CONFIG _IOW(HLK_LD2420_IOCTL_MAGIC, 2, int)

static struct i2c_client *hlk_ld2420_client;
static struct class* hlk_ld2420_class = NULL;
static struct device* hlk_ld2420_device = NULL;
static int major_number;

// Function to read data from HLK-LD2420
static int hlk_ld2420_read_data(struct i2c_client *client)
{
    u8 buf[2];
    int radar_data;

    // Read radar data (2 bytes)
    if (i2c_smbus_read_i2c_block_data(client, HLK_LD2420_REG_DATA, sizeof(buf), buf) < 0) {
        printk(KERN_ERR "Failed to read radar data\n");
        return -EIO;
    }

    // Combine high and low bytes
    radar_data = (buf[0] << 8) | buf[1];
    return radar_data;
}

// Function to configure HLK-LD2420
static int hlk_ld2420_set_config(struct i2c_client *client, int config)
{
    // Write config value to the radar (1 byte)
    if (i2c_smbus_write_byte_data(client, HLK_LD2420_REG_CONFIG, config) < 0) {
        printk(KERN_ERR "Failed to write radar configuration\n");
        return -EIO;
    }

    return 0;
}

// IOCTL handler
static long hlk_ld2420_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int data, ret;

    switch (cmd) {
        case HLK_LD2420_IOCTL_READ_DATA:
            data = hlk_ld2420_read_data(hlk_ld2420_client);
            if (data < 0)
                return data;
            if (copy_to_user((int __user *)arg, &data, sizeof(data)))
                return -EFAULT;
            break;

        case HLK_LD2420_IOCTL_SET_CONFIG:
            if (copy_from_user(&data, (int __user *)arg, sizeof(data)))
                return -EFAULT;
            ret = hlk_ld2420_set_config(hlk_ld2420_client, data);
            if (ret < 0)
                return ret;
            break;

        default:
            return -EINVAL;
    }

    return 0;
}

// File operations
static int hlk_ld2420_open(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "HLK-LD2420 device opened\n");
    return 0;
}

static int hlk_ld2420_release(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "HLK-LD2420 device closed\n");
    return 0;
}

static struct file_operations fops = {
    .open = hlk_ld2420_open,
    .unlocked_ioctl = hlk_ld2420_ioctl,
    .release = hlk_ld2420_release,
};

// Probe function
static int hlk_ld2420_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
hlk_ld2420_client = client;

    // Register char device
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ERR "Failed to register a major number\n");
        return major_number;
    }

    hlk_ld2420_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(hlk_ld2420_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ERR "Failed to register device class\n");
        return PTR_ERR(hlk_ld2420_class);
    }

    hlk_ld2420_device = device_create(hlk_ld2420_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(hlk_ld2420_device)) {
        class_destroy(hlk_ld2420_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ERR "Failed to create the device\n");
        return PTR_ERR(hlk_ld2420_device);
    }

    printk(KERN_INFO "HLK-LD2420 driver installed\n");
    return 0;
}

// Remove function
static void hlk_ld2420_remove(struct i2c_client *client)
{
    device_destroy(hlk_ld2420_class, MKDEV(major_number, 0));
    class_unregister(hlk_ld2420_class);
    class_destroy(hlk_ld2420_class);
    unregister_chrdev(major_number, DEVICE_NAME);

    printk(KERN_INFO "HLK-LD2420 driver removed\n");
}

// Device ID table
static const struct i2c_device_id hlk_ld2420_id[] = {
    { "hlk_ld2420", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, hlk_ld2420_id);

// I2C driver structure
static struct i2c_driver hlk_ld2420_driver = {
    .driver = {
        .name   = DRIVER_NAME,
        .owner  = THIS_MODULE,
    },
    .probe      = hlk_ld2420_probe,
    .remove     = hlk_ld2420_remove,
    .id_table   = hlk_ld2420_id,
};

// Module init and exit
static int __init hlk_ld2420_init(void)
{
    printk(KERN_INFO "Initializing HLK-LD2420 driver\n");
    return i2c_add_driver(&hlk_ld2420_driver);
}

static void __exit hlk_ld2420_exit(void)
{
    printk(KERN_INFO "Exiting HLK-LD2420 driver\n");
    i2c_del_driver(&hlk_ld2420_driver);
}

module_init(hlk_ld2420_init);
module_exit(hlk_ld2420_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("HLK-LD2420 I2C Client Driver with IOCTL Interface");
MODULE_VERSION("1.0");
