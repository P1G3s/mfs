#include <linux/fs.h>       /* file stuff */
#include <linux/kernel.h>    /* printk() */
#include <linux/errno.h>     /* error codes */
#include <linux/module.h>    /* THIS_MODULE */
#include <linux/cdev.h>      /* char device stuff */
#include <asm/uaccess.h>     /* strncpy_from_user() */
#include <linux/slab.h>
#include "swap_module.h"

MODULE_LICENSE("GPL");

int register_device(void);
void unregister_device(void);

static int my_init(void){
	printk(KERN_ALERT "INIT\n");
	register_device();	
//	ino_init();
	return 0;
}

static void my_exit(void){
	printk(KERN_ALERT "TERMINATED\n");
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
	int src_len, des_len;
	char* sep; char* src_name; char* des_name;
	printk(KERN_NOTICE "SWAP_DRIVER: device file has been written with '%s'\n", buffer);
	if (!(sep = strchr(buffer, ' '))){
		printk(KERN_NOTICE "SWAP_DRIVER: wrong input format");
		length = -1;
		goto finish;
	}
	else{
		src_len = sep-buffer;
		des_len = strchr(sep+1,'\n') ? (buffer+length)-sep-2 : (buffer+length)-sep-1;
		src_name = (char*) kmalloc(sizeof(char) * (src_len), GFP_KERNEL);
		des_name = (char*) kmalloc(sizeof(char) * (des_len), GFP_KERNEL);
		if (strncpy_from_user(src_name, buffer, src_len) == -EFAULT){
			printk(KERN_NOTICE "SWAP_DRIVER: failed to get buffer from user\n");
			length = -1;
			goto finish;
		}
		if (strncpy_from_user(des_name, sep+1, des_len) == -EFAULT){
			printk(KERN_NOTICE "SWAP_DRIVER: failed to get buffer from user\n");
			length = -1;
			goto finish;
		}
	}
	printk(KERN_NOTICE "SWAP_DRIVER: write src_name :'%s'\n", src_name);
	printk(KERN_NOTICE "SWAP_DRIVER: write des_name :'%s'\n", des_name);
	ino_swap(src_name, des_name);
	kfree(src_name);
	kfree(des_name);

finish:
	return length;
}


// REGISTER & UNREGISTER
static struct file_operations swap_fops = {
	.owner = THIS_MODULE,
	.write = device_file_write,
};
static int device_file_major_number=0;
static const char device_name[]="SWAP_DRIVER";

int register_device(void){
	int result = 0;
	//printk( KERN_NOTICE "SWAP_DRIVER: register_device() is called.\n");
	result = register_chrdev( 0, device_name, &swap_fops );
	if ( result<0 ){
		printk( KERN_WARNING "SWAP_DRIVER: cant register char device with error code = %i\n", result );
		return result;
	}
	device_file_major_number = result;
	printk( KERN_NOTICE "SWAP_DRIVER: registered char device with major number = %i\n", device_file_major_number );
	return 0;
}

void unregister_device(void){
	//printk( KERN_NOTICE "SWAP_DRIVER: unregsiter_device() is called\n");
	if ( device_file_major_number != 0 ){
		unregister_chrdev( device_file_major_number, device_name );
	}
}


