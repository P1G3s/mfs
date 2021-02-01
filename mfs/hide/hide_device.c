#include <linux/fs.h>       /* file stuff */
#include <linux/kernel.h>    /* printk() */
#include <linux/errno.h>     /* error codes */
#include <linux/module.h>    /* THIS_MODULE */
#include <linux/cdev.h>      /* char device stuff */
#include <asm/uaccess.h>     /* strncpy_from_user() */
#include <linux/uaccess.h>
#include <linux/slab.h>
#include "hide_module.h"

MODULE_LICENSE("GPL");

int hide_register_device(void);
void hide_unregister_device(void);

static int my_init(void){
	printk(KERN_ALERT "HIDE_DRIVER: ######## INIT ########\n");
	hide_register_device();	
	return 0;
}

static void my_exit(void){
	hide_unregister_device();
	printk(KERN_ALERT "HIDE_DRIVER: ##### TERMINATED #####\n\n\n");
	return;
}

module_init(my_init);
module_exit(my_exit);

// FILE OPERATIONS
static ssize_t device_file_read( struct file *file_ptr,
									char __user *user_buffer,
									size_t count,
									loff_t *position){
}

static ssize_t device_file_write( struct file *file_ptr,
									const char *buffer,
									size_t length,
									loff_t *position){
	int src_len;
	char* src_name;
	printk(KERN_NOTICE "HIDE_DRIVER: Write '%s'\n", buffer);
	// CHECK HIDEPED REQUEST
	if (buffer[0] == 'H'){
		buffer += 1;
		length -= 1;
		src_len = strchr(buf, '\n') ? length-1 : length;
		src_name = (char*) kmalloc(sizeof(char) * (src_len), GFP_KERNEL);
		if (strncpy_from_user(src_name, buffer, src_len) == -EFAULT){
			printk(KERN_NOTICE "HIDE_DRIVER: Failed to get buffer from user\n");
			length = -1;
		}
		// #HIDE CODE HERE#
		kfree(src_name);
	}
	else if (buffer[0] == 'R'){
		// #RECOVER CODE HERE#
	}
}


// REGISTER & UNREGISTER
static struct file_operations hide_fops = {
	.owner = THIS_MODULE,
	.write = device_file_write,
	.read = device_file_read,
};
static int device_file_major_number=0;
static const char device_name[]="HIDE_DRIVER";

int hide_register_device(void){
	int result = 0;
	result = register_chrdev( 0, device_name, &hide_fops );
	if ( result<0 ){
		printk( KERN_WARNING "HIDE_DRIVER: CAN NOT register char device with error code = %i\n", result );
		return result;
	}
	device_file_major_number = result;
	printk( KERN_NOTICE "HIDE_DRIVER: Registered, Major number = %i\n", device_file_major_number );
	return 0;
}

void hide_unregister_device(void){
	if ( device_file_major_number != 0 ){
		unregister_chrdev( device_file_major_number, device_name );
	}
}


