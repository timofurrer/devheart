#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

// module header information
MODULE_LICENSE("MIT");
MODULE_AUTHOR("Timo Furrer");
MODULE_DESCRIPTION("Kernel Module which illustrates a Tuxs heart. ");


// device name to use
#define DEVICE_NAME "heart"

static int device_open(struct inode *inode, struct file *file) {
    pr_info("heart device open");
    return 0;
}

static int device_release(struct inode *inode, struct file *file) {
    pr_info("heart device release");
    return 0;
}

static ssize_t device_read(struct file *file, char *buffer, size_t length, loff_t *offset) {
    pr_info("heart device read");
    return 0;
}

static ssize_t device_write(struct file *file, const char *buffer, size_t length, loff_t *offset) {
    pr_info("heart device write");
    return 0;
}

static const struct file_operations fileops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release
};

static struct miscdevice heart_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &fileops,
    .mode = S_IRUGO,
};

static int __init heart_init(void)
{
    int ret;

    ret = misc_register(&heart_dev);
    if(ret) {
        pr_err("could not register heart device as misc devie\n");
        return ret;
    }

    pr_info("registered heart device!\n");

    return 0;
}

static void __exit heart_exit(void)
{
    misc_deregister(&heart_dev);
    pr_info("deregistered heart device!\n");
        /*printk(KERN_INFO "Cleaning up module.\n");*/
}

module_init(heart_init);
module_exit(heart_exit);
