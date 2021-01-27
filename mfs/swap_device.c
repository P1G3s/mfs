#include <linux/fs.h>       /* file stuff */
#include <linux/kernel.h>    /* printk() */
#include <linux/errno.h>     /* error codes */
#include <linux/module.h>    /* THIS_MODULE */
#include <linux/cdev.h>      /* char device stuff */
#include <asm/uaccess.h>     /* strncpy_from_user() */
#include "swap_module.h"

MODULE_LICENSE("GPL");

int register_device(void);
void unregister_device(void);

static int my_init(void){
	printk(KERN_ALERT "HELLO\n");
	register_device();	
	ino_swap();
	return 0;
}

static void my_exit(void){
	printk(KERN_ALERT "GOODBYE\n");
	unregister_device();
	return;
}

module_init(my_init);
module_exit(my_exit);

// FILE OPERATIONS
static ssize_t device_file_write( struct file *file_ptr,
									const char *buffer,
									size_t length,
									loff_t *position){
/*	printk( KERN_NOTICE "swap_driver: Device file is written at offset = %i, write bytes length = %u, content = %s\n",
			(int)* position,
			(unsigned int) length,
			buffer );
			*/
	printk( KERN_NOTICE "WRITE\n");
	ino_swap();
	return length;
}


// REGISTER & UNREGISTER
static struct file_operations swap_fops = {
	.owner = THIS_MODULE,
	.write = device_file_write,
};
static int device_file_major_number=0;
static const char device_name[]="swap_driver";

int register_device(void){
	int result = 0;
	printk( KERN_NOTICE "swap_driver: register_device() is called.\n");
	result = register_chrdev( 0, device_name, &swap_fops );
	if ( result<0 ){
		printk( KERN_WARNING "swap_driver: cant register char device with error code = %i\n", result );
		return result;
	}
	device_file_major_number = result;
	printk( KERN_NOTICE "swap_driver: registered char device with major number = %i and minor number 0...255\n",
device_file_major_number );
	return 0;
}

void unregister_device(void){
	printk( KERN_NOTICE "swap_driver: unregsiter_device() is called\n");
	if ( device_file_major_number != 0 ){
		unregister_chrdev( device_file_major_number, device_name );
	}
}


