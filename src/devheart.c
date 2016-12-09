/**
 *
 * Kernel Module which creates a device to listen to Tuxs heart.
 *
 * @copyright: GPLv2 (see LICENSE), by Timo Furrer <tuxtimo@gmail.com>
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
#include <linux/cpumask.h>  // for_each_possible_cpu
#include <linux/kernel_stat.h>  // kcpustat_cpu
#include <linux/delay.h>  // msleep_interruptible
#include <linux/tick.h> // get_cpu_idle_time_us

#include "devheart.h"

// module header information
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Timo Furrer");
MODULE_DESCRIPTION("Kernel Module which illustrates a Tuxs heart.");

// device name to use
#define DEVICE_NAME "heart"

// interval in milliseconds in which to measure CPU utilization
#define CPU_MEASURE_INTERVAL 1000

// heart beat sound data
extern struct devheart_sound_t single_beat;

// kernel thread instance
struct task_struct *task;

// current data which is being read
// FIXME: refactor - find better way to do this ...
struct devheart_read_data_t {
    char *data;
};

static u64 get_idle_time(int cpu)
{
    u64 idle, idle_time = -1ULL;

    if (cpu_online(cpu)) {
        idle_time = get_cpu_idle_time_us(cpu, NULL);
    }

    if (idle_time == -1ULL) {
        /* !NO_HZ or cpu offline so we can rely on cpustat.idle */
        idle = kcpustat_cpu(cpu).cpustat[CPUTIME_IDLE];
    }
    else {
        idle = usecs_to_cputime64(idle_time);
    }

    return idle;
}

static u64 get_iowait_time(int cpu)
{
    u64 iowait, iowait_time = -1ULL;

    if (cpu_online(cpu)) {
        iowait_time = get_cpu_iowait_time_us(cpu, NULL);
    }

    if (iowait_time == -1ULL) {
        /* !NO_HZ or cpu offline so we can rely on cpustat.iowait */
        iowait = kcpustat_cpu(cpu).cpustat[CPUTIME_IOWAIT];
    }
    else {
        iowait = usecs_to_cputime64(iowait_time);
    }

    return iowait;
}

void cpu_stat(u64 *idle_time, u64 *total_time) {
    int i;
    u64 user, nice, system, idle, iowait, irq, softirq, steal = 0;

    for_each_possible_cpu(i) {
        user += kcpustat_cpu(i).cpustat[CPUTIME_USER];
        nice += kcpustat_cpu(i).cpustat[CPUTIME_NICE];
        system += kcpustat_cpu(i).cpustat[CPUTIME_SYSTEM];
        idle += get_idle_time(i);
        iowait += get_iowait_time(i);
        irq += kcpustat_cpu(i).cpustat[CPUTIME_IRQ];
        softirq += kcpustat_cpu(i).cpustat[CPUTIME_SOFTIRQ];
        steal += kcpustat_cpu(i).cpustat[CPUTIME_STEAL];
    }

    *idle_time = idle + iowait;
    *total_time = *idle_time + user + nice + system + irq + softirq + steal;
}

int measure_cpu_utilization(void *data) {
    static u64 previous_cpu_idle_time = 0;
    static u64 previous_cpu_total_time = 0;

    u64 current_cpu_idle_time, current_cpu_total_time = 0;
    u64 delta_idle_time, delta_total_time = 0;

    // current CPU utilization in percentage
    int cpu_utilization = 0;

    // initial fetch of cpu times
    cpu_stat(&previous_cpu_idle_time, &previous_cpu_total_time);

    while(!kthread_should_stop()) {
        msleep_interruptible(CPU_MEASURE_INTERVAL);

        // get current CPU stats
        cpu_stat(&current_cpu_idle_time, &current_cpu_total_time);

        // calculate CPU stat difference since last measurement
        delta_idle_time = current_cpu_idle_time - previous_cpu_idle_time;
        delta_total_time = current_cpu_total_time - previous_cpu_total_time;

        // calculate CPU usage in percentage
        cpu_utilization = (1000 * (delta_total_time - delta_idle_time) / delta_total_time + 5) / 10;
        pr_info("Current CPU Utilization is %d%%", cpu_utilization);

        previous_cpu_idle_time = current_cpu_idle_time;
        previous_cpu_total_time = current_cpu_total_time;
    }

    return 0;
}

static int device_open(struct inode *inode, struct file *file) {
    pr_info("heart device open");
    struct devheart_read_data_t *read_data;

    read_data = kzalloc(sizeof(*read_data), GFP_KERNEL);
    if(!read_data) {
        pr_err("could not allocate kernel memory for heartbeat read data");
        return -ENOMEM;
    }

    // store current data in context object
    read_data->data = single_beat.data;

    // store context object
    file->private_data = read_data;
    return 0;
}

static int device_release(struct inode *inode, struct file *file) {
    struct devheart_read_data_t *read_data = file->private_data;

    kfree(read_data);

    pr_info("heart device release");
    return 0;
}

static ssize_t device_read(struct file *file, char *buffer, size_t length, loff_t *offset) {
    int read;
    struct devheart_read_data_t *current_data;

    // the number of bytes which are already been read
    read = 0;

    current_data = file->private_data;

    // read sound track until completed.
    while(length && (current_data->data - single_beat.data) < single_beat.size) {
        put_user(*(current_data->data++), buffer++);
        length--;
        read++;
    }

    return read;
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

    pr_info("Sound length: %lu", single_beat.size);
    pr_info("Sound data[0]: %x", single_beat.data[0]);

    // TODO: check return value
    task = kthread_run(&measure_cpu_utilization, NULL, "heartmonitor");
    pr_info("start kthread %s to measure cpu utilization", task->comm);

    ret = misc_register(&heart_dev);
    if(ret) {
        pr_err("could not register heart device as misc devie\n");
        return ret;
    }

    pr_info("Listen to Tux's heart!");
    pr_info("--> cat /dev/" DEVICE_NAME " | aplay -r 62500");

    return 0;
}

static void __exit heart_exit(void)
{
    kthread_stop(task);

    misc_deregister(&heart_dev);
    pr_info("deregistered heart device!\n");
}

module_init(heart_init);
module_exit(heart_exit);
