/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/slab.h>
#include "aesdchar.h"
int aesd_major = 0; // use dynamic major
int aesd_minor = 0;

MODULE_AUTHOR("Chakshu Bhardwaj"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    filp->private_data = container_of(inode->i_cdev, struct aesd_dev, cdev);

    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                  loff_t *f_pos)
{
    ssize_t retval = 0;
    struct aesd_dev *dev;
    struct aesd_buffer_entry *p_entry;
    size_t entry_offset = 0ULL;

    PDEBUG("read %zu bytes with offset %lld", count, *f_pos);
    /**
     * TODO: handle read
     */
    /* Check input */
    if ((filp == NULL) || (f_pos == NULL))
    {
        return -EINVAL;
    }
    if (!access_ok(buf, count))
    {
        return -EINVAL;
    }

    dev = filp->private_data;
    if (dev == NULL)
    {
        printk("Null dev struct\n");
        return -EINVAL;
    }
    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    p_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->circular_buffer, *f_pos, &entry_offset);

    if (p_entry == NULL)
    {
        PDEBUG("Position exceeds buffer size");
        goto out;
    }

    if (count > (p_entry->size - entry_offset))
    {
        count = p_entry->size - entry_offset;
    }

    if (copy_to_user(buf, p_entry->buffptr + entry_offset, count))
    {
        retval = -EFAULT;
        goto out;
    }

    *f_pos += count;
    retval = count;
out:
    mutex_unlock(&dev->lock);
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                   loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    struct aesd_dev *dev;
    const char *presult;
    PDEBUG("write %zu bytes with offset %lld", count, *f_pos);
    /**
     * TODO: handle write
     */

    if ((filp == NULL) || (f_pos == NULL))
    {
        return -EINVAL;
    }
    if (!access_ok(buf, count))
    {
        return -EINVAL;
    }

    dev = filp->private_data;

    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    presult = krealloc(dev->entry.buffptr, dev->entry.size + count, GFP_KERNEL);
    if (presult == NULL)
    {
        PDEBUG("Could not allocate memory for circular buffer entry");
        retval = -ENOMEM;
        goto leave;
    }

    if (copy_from_user((char *)presult + dev->entry.size, buf, count))
    {
        PDEBUG("Couldnt copy fron user buffer");
        retval = -EFAULT;
        goto leave;
    }

    dev->entry.buffptr = presult;
    dev->entry.size += count;

    if (memchr(dev->entry.buffptr, '\n', dev->entry.size))
    {
        const char *replaced_buffer = aesd_circular_buffer_add_entry(&(dev->circular_buffer), &(dev->entry));

        kfree(replaced_buffer);

        dev->entry.buffptr = NULL;
        dev->entry.size = 0;
    }

    *f_pos += count;
    retval = count;
leave:
    mutex_unlock(&dev->lock);
    return retval;
}
struct file_operations aesd_fops = {
    .owner = THIS_MODULE,
    .read = aesd_read,
    .write = aesd_write,
    .open = aesd_open,
    .release = aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
    {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}

int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
                                 "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0)
    {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device, 0, sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */
    aesd_circular_buffer_init(&aesd_device.circular_buffer);
    mutex_init(&aesd_device.lock);
    printk("Mutex initialized\n");

    result = aesd_setup_cdev(&aesd_device);

    if (result)
    {
        unregister_chrdev_region(dev, 1);
    }
    return result;
}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */
    mutex_destroy(&aesd_device.lock);

    unregister_chrdev_region(devno, 1);
}

module_init(aesd_init_module);
module_exit(aesd_cleanup_module);