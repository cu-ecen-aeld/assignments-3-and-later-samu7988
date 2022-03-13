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
#include <linux/slab.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Sayali Mule"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
	struct aesd_dev *dev; /* device information */

	PDEBUG("open");
	/**
	 * TODO: handle open
	 */
	dev = container_of(inode->i_cdev, struct aesd_dev, cdev); //retrieves pointer to struct aesd_dev type which will be used in other methods
	filp->private_data = dev; /* for other methods */

	return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
	PDEBUG("release");
	/**
	 * TODO: handle release
	 */
	filp->private_data = NULL;
	return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = 0;
	size_t offset_within_particular_entry =0;
	size_t remaining_bytes_to_read_in_entry = 0;
	struct aesd_buffer_entry* entry_within_cb_p; //Points to particular entry in circular buffer
	size_t remaining_number_of_bytes_still_to_be_copied = 0;

	//Get the pointer to aesd_dev which contains the circular buffer
	struct aesd_dev* dev_p = (struct aesd_dev*)(filp->private_data);
	if(dev_p == NULL)
	{
		goto error_handler;
	}

	//Get circular buffer pointer from dev_p
	struct aesd_circular_buffer* cb_handler_p = &(dev_p->cb_handler);
	if(cb_handler_p == NULL)
	{
		goto error_handler;
	}

	//Get particular entry within circular buffer depending upon *fpos
	entry_within_cb_p = aesd_circular_buffer_find_entry_offset_for_fpos(cb_handler_p,*f_pos,&offset_within_particular_entry);
	if(entry_within_cb_p == NULL)
	{
		goto error_handler;
	}
	
	remaining_bytes_to_read_in_entry = entry_within_cb_p->size - offset_within_particular_entry;

	//Prevent to read more than what user has requested
	if(remaining_bytes_to_read_in_entry > count)
	{
		remaining_bytes_to_read_in_entry = count;
	}
	
	const char* string_to_be_read_from_kernel = entry_within_cb_p->buffptr + offset_within_particular_entry;
	if(string_to_be_read_from_kernel == NULL)
	{
		goto error_handler;
	}

	remaining_number_of_bytes_still_to_be_copied = copy_to_user(buf,string_to_be_read_from_kernel, remaining_bytes_to_read_in_entry);

	retval = remaining_bytes_to_read_in_entry - remaining_number_of_bytes_still_to_be_copied;
	*f_pos += remaining_bytes_to_read_in_entry - remaining_number_of_bytes_still_to_be_copied;;

	
	PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
	/**
	 * TODO: handle read
	 */

	PDEBUG("Read string %s",str);
	
error_handler:
	return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = -ENOMEM;
	//Get the pointer to aesd_dev which contains the circular buffer
	struct aesd_dev* dev_p = (struct aesd_dev*)(filp->private_data);
	
	//Get circular buffer pointer
	struct aesd_circular_buffer* cb_handler = &(dev_p->cb_handler);

	char* temp_string_p = kmalloc(strlen(buf) + 1,GFP_KERNEL);
	struct aesd_buffer_entry entry;

	PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
	/**
	 * TODO: handle write
	 */

	retval = copy_from_user(temp_string_p,buf,strlen(buf)+1);

	entry.buffptr = temp_string_p;
	entry.size = strlen(temp_string_p);

	aesd_circular_buffer_add_entry(cb_handler,&entry);

	//retval = strlen(temp_string_p) + 1;


	return retval;
}
struct file_operations aesd_fops = {
	.owner =    THIS_MODULE,
	.read =     aesd_read,
	.write =    aesd_write,
	.open =     aesd_open,
	.release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
	int err, devno = MKDEV(aesd_major, aesd_minor);

	cdev_init(&dev->cdev, &aesd_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &aesd_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	if (err) {
		printk(KERN_ERR "Error %d adding aesd cdev", err);
	}
	return err;
}



int aesd_init_module(void)
{
	dev_t dev = 0;
	int result;
	//Populate device number in first parameter i.e dev(12 bit major , 20bit minor)
	result = alloc_chrdev_region(&dev, aesd_minor, 1,
			"aesdchar");
	aesd_major = MAJOR(dev);
	if (result < 0) {
		printk(KERN_WARNING "Can't get major %d\n", aesd_major);
		return result;
	}
	memset(&aesd_device,0,sizeof(struct aesd_dev));

	/**
	 * TODO: initialize the AESD specific portion of the device
	 */
	aesd_circular_buffer_init(&aesd_device.cb_handler);
	
	result = aesd_setup_cdev(&aesd_device);

	if( result ) {
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

	unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
