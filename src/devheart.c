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
#include <linux/vmalloc.h> // vmalloc

#include "devheart.h"

// module header information
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Timo Furrer");
MODULE_DESCRIPTION("Kernel Module to listen to Tuxs heart.");

// device name to use
#define DEVICE_NAME "heart"

// interval in milliseconds in which to measure CPU utilization
#define CPU_MEASURE_INTERVAL 1000

// heart beat sound data
extern struct devheart_sound_t left_ventricle_beat_sound;
extern struct devheart_sound_t right_ventricle_beat_sound;

// kernel thread instance
struct task_struct *task;

// current data which is being read
// FIXME: refactor - find better way to do this ...
struct devheart_sound_buffer_t {
    char *buffer;
    size_t size;
    size_t current_offset;
};

const char PAUSE_SOUND_BYTE = 0xFF;

// current cpu utilization
int current_cpu_utilization = 0;

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
        pr_info("current CPU utilization is %d%%\n", cpu_utilization);
        current_cpu_utilization = cpu_utilization;

        previous_cpu_idle_time = current_cpu_idle_time;
        previous_cpu_total_time = current_cpu_total_time;
    }

    return 0;
}

static ssize_t generate_heartbeat(struct devheart_sound_buffer_t *sound_buffer) {
    size_t data_size;
    int utilization_factor, short_pause_factor, long_pause_factor;
    size_t offset = 0;
    int i;

    pr_debug("=====> Generating new heartbeat ...\n");

    // TODO: experiment and improve!
    utilization_factor = (100 - current_cpu_utilization) / 6;
    short_pause_factor = utilization_factor;
    long_pause_factor = utilization_factor * 60;

    // beats
    data_size = left_ventricle_beat_sound.size + right_ventricle_beat_sound.size;

    // pause between the two beats
    data_size += 64 * short_pause_factor;

    // pause after the two beats
    data_size += 64 * long_pause_factor;

    // generate heartbeat
    sound_buffer->buffer = vzalloc(data_size * sizeof(char));
    if(!sound_buffer->buffer) {
        pr_err("could not allocate kernel memory for next heartbeat -> defibrillate, NOW!\n");
        return -ENOMEM;
    }

    // write sound data to buffer
    memcpy(sound_buffer->buffer, left_ventricle_beat_sound.data, left_ventricle_beat_sound.size);
    offset += left_ventricle_beat_sound.size;

    for(i = 0; i < short_pause_factor * 64; i++) {
        memcpy(sound_buffer->buffer + offset, &PAUSE_SOUND_BYTE, sizeof(PAUSE_SOUND_BYTE));
        offset += sizeof(PAUSE_SOUND_BYTE);
    }

    memcpy(sound_buffer->buffer + offset, right_ventricle_beat_sound.data, right_ventricle_beat_sound.size);
    offset += right_ventricle_beat_sound.size;

    for(i = 0; i < long_pause_factor * 64; i++) {
        memcpy(sound_buffer->buffer + offset, &PAUSE_SOUND_BYTE, sizeof(PAUSE_SOUND_BYTE));
        offset += sizeof(PAUSE_SOUND_BYTE);
    }

    sound_buffer->size = data_size;
    sound_buffer->current_offset = 0;
    return data_size;
}

static int device_open(struct inode *inode, struct file *file) {
    struct devheart_sound_buffer_t *sound_buffer;

    pr_info("Okay, let's listen to Master Tuxs heart ...\n");

    sound_buffer = kzalloc(sizeof(*sound_buffer), GFP_KERNEL);
    if(!sound_buffer) {
        pr_err("could not allocate kernel memory for heartbeat read data\n");
        return -ENOMEM;
    }

    // generate first heartbeat on open to be ready when it staaaarts!
    generate_heartbeat(sound_buffer);

    // store context object
    file->private_data = sound_buffer;
    return 0;
}

static int device_release(struct inode *inode, struct file *file) {
    struct devheart_sound_buffer_t *sound_buffer = file->private_data;

    kfree(sound_buffer);

    pr_info("I'll check in on you later, Master Tux!\n");
    return 0;
}

static ssize_t device_read(struct file *file, char *buffer, size_t length, loff_t *offset) {
    struct devheart_sound_buffer_t *sound_buffer;
    size_t bytes_read;

    // read sound buffer
    sound_buffer = file->private_data;

    // check if current sound buffer is exhausted
    if(sound_buffer->size - sound_buffer->current_offset <= 0) {
        // generate next heartbeat including pauses
        vfree(sound_buffer->buffer - sound_buffer->current_offset);
        generate_heartbeat(sound_buffer);
    }

    bytes_read = 0;
    while(length && (sound_buffer->size - sound_buffer->current_offset) > 0) {
        put_user(*(sound_buffer->buffer++), buffer++);
        length--;
        sound_buffer->current_offset++;
        bytes_read++;
    }

    *offset = bytes_read;
    return bytes_read;
}

static ssize_t device_write(struct file *file, const char *buffer, size_t length, loff_t *offset) {
    pr_err("I'm sooo sorry, but you cannot influence Master Tux' healt ...\n");
    return -EINVAL;
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

    ret = misc_register(&heart_dev);
    if(ret) {
        pr_err("could not register heart device as misc devie\n");
        return ret;
    }

    pr_info("Listen to Tux's heart!\n");
    pr_info("--> cat /dev/" DEVICE_NAME " | aplay -r 44100 -f s16_le\n");

    return 0;
}

static void __exit heart_exit(void)
{
    kthread_stop(task);

    misc_deregister(&heart_dev);
}

module_init(heart_init);
module_exit(heart_exit);
