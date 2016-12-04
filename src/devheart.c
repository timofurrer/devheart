/**
 *
 * Kernel Module which creates a device to listen to Tuxs heart.
 *
 * @copyright: MIT (see LICENSE), by Timo Furrer <tuxtimo@gmail.com>
 *
 */

// use kernel module name in from of kernel log messages
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/sched.h>

// module header information
MODULE_LICENSE("MIT");
MODULE_AUTHOR("Timo Furrer");
MODULE_DESCRIPTION("Kernel Module which illustrates a Tuxs heart.");

// device name to use
#define DEVICE_NAME "heart"

// kernel thread instance
struct task_struct *task;

int measure_cpu_utilization(void *data) {
    pr_info("in thread function");

    while(!kthread_should_stop()) {
        pr_info("in kthread function loop");
        schedule();
    }

    return 0;
}

static int device_open(struct inode *inode, struct file *file) {
    pr_info("heart device open");
    return 0;
}

static int device_release(struct inode *inode, struct file *file) {
    pr_info("heart device release");
    return 0;
}

static ssize_t device_read(struct file *file, char *buffer, size_t length, loff_t *offset) {
    char *data = "Master, tux is very healthy!\n";
    int len = strlen(data);
    /*
     * only support reading the whole string at once.
     */
    if(length < len) {
        return -EINVAL;
    }
    /*
     * If file position is non-zero, then assume the string has
     * been read and indicate there is no more data to be read.
     */
    if(*offset != 0) {
        return 0;
    }
    /*
     * Besides copying the string to the user provided buffer,
     * this function also checks that the user has permission to
     * write to the buffer, that it is mapped, etc.
     */
    if(copy_to_user(buffer, data, len))
            return -EINVAL;
    /*
     * Tell the user how much data we wrote.
     */
    *offset = len;

    return len;
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

    // TODO: check return value
    task = kthread_run(&measure_cpu_utilization, NULL, "heartmonitor");
    pr_info("start kthread %s to measure cpu utilization", task->comm);

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

    kthread_stop(task);
}

module_init(heart_init);
module_exit(heart_exit);
