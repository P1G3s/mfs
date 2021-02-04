#include <linux/fs.h>       /* file stuff */
#include <linux/kernel.h>    /* printk() */
#include <linux/errno.h>     /* error codes */
#include <linux/module.h>    /* THIS_MODULE */
#include <linux/cdev.h>      /* char device stuff */
#include <asm/uaccess.h>     /* strncpy_from_user() */
#include <linux/uaccess.h>
#include <linux/slab.h>
#include "swap_module.h"

MODULE_LICENSE("GPL");

int swap_flag = 1;
int swap_register_device(void);
void swap_unregister_device(void);

static int my_init(void){
	printk(KERN_ALERT "SWAP_DRIVER: ######## INIT ########\n");
	swap_register_device();	
	ino_init();
	return 0;
}

static void my_exit(void){
	ino_recover();
	swap_unregister_device();
	printk(KERN_ALERT "SWAP_DRIVER: ##### TERMINATED #####\n\n\n");
	return;
}

module_init(my_init);
module_exit(my_exit);

// FILE OPERATIONS
static ssize_t device_file_read( struct file *file_ptr,
									char __user *user_buffer,
									size_t count,
									loff_t *position){
	char read_val;

	read_val = swap_flag ? 'Y':'N';
	printk(KERN_ALERT "SWAP_DRIVER: Read '%c'\n", read_val);
	if (copy_to_user(user_buffer, &read_val, 1) != 0)
		return -EFAULT;
	return count;
}

static ssize_t device_file_write( struct file *file_ptr,
									const char *buffer,
									size_t length,
									loff_t *position){
	int src_len, des_len;
	char* sep; char* src_name; char* des_name;
	char is_swapped;

	printk(KERN_ALERT "SWAP_DRIVER: Write '%s'\n", buffer);
	// CHECK SWAPPED REQUEST
	if (buffer[0] == 'C'){
		buffer += 1;
	  	length -= 1;
		src_len = strchr(buffer,'\n') ? length-1 : length;
		src_name = (char*) kmalloc(sizeof(char) * (src_len), GFP_KERNEL);
		if (strncpy_from_user(src_name, buffer, src_len) == -EFAULT){
			printk(KERN_WARNING "SWAP_DRIVER: Failed to get buffer from user\n");
			length = -1;
			goto finish;
		}
		swap_flag = ino_is_swapped(src_name);	
		is_swapped = swap_flag ? 'Y':'N';
		printk(KERN_NOTICE "SWAP_DRIVER: Is it swapped? -> '%c'\n", is_swapped);
		kfree(src_name);
		length += 1;
	}
	// SWAP FILE REQUEST
	else if (buffer[0] == 'S'){
		sep = strchr(buffer, ' ');
		buffer += 1;
		length -= 1;
		src_len = sep-buffer;
		des_len = strchr(buffer, '\n') ? (buffer+length)-sep-2 : (buffer+length)-sep-1;
		src_name = (char*) kmalloc(sizeof(char) * (src_len), GFP_KERNEL);
		des_name = (char*) kmalloc(sizeof(char) * (des_len), GFP_KERNEL);
		if (strncpy_from_user(src_name, buffer, src_len) == -EFAULT){
			printk(KERN_WARNING "SWAP_DRIVER: Failed to get buffer from user\n");
			length = -1;
			goto finish;
		}
		if (strncpy_from_user(des_name, sep+1, des_len) == -EFAULT){
			printk(KERN_WARNING "SWAP_DRIVER: Failed to get buffer from user\n");
			length = -1;
			goto finish;
		}
		ino_swap(src_name, des_name);
		kfree(src_name);
		kfree(des_name);
		length += 1;
	}
	// RECOVER
	else {
		ino_recover();
	}
finish:
	return length;
}


// REGISTER & UNREGISTER
static struct file_operations swap_fops = {
	.owner = THIS_MODULE,
	.write = device_file_write,
	.read = device_file_read,
};
static int device_file_major_number=0;
static const char device_name[]="SWAP_DRIVER";

int swap_register_device(void){
	int result = 0;
	result = register_chrdev( 0, device_name, &swap_fops );
	if ( result<0 ){
		printk( KERN_WARNING "SWAP_DRIVER: CAN NOT register char device with error code = %i\n", result );
		return result;
	}
	device_file_major_number = result;
	printk( KERN_NOTICE "SWAP_DRIVER: Registered. Major number = %i\n", device_file_major_number );
	return 0;
}

void swap_unregister_device(void){
	//printk( KERN_NOTICE "SWAP_DRIVER: unregsiter_device() is called\n");
	if ( device_file_major_number != 0 ){
		unregister_chrdev( device_file_major_number, device_name );
	}
}


