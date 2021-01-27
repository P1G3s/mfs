#include "swap_module.h"

#define SWAP(x,y,temp) {temp=x; x=y; y=temp;}

int ino_swap(const char* src_name, const char* des_name){
	struct path src_path;
	struct path des_path;
	struct inode* temp;
	int ret = 0;

	ret = kern_path(src_name, LOOKUP_FOLLOW, &src_path);
	if (ret) {pr_err("swap_driver: Failed to look up source directory, err:%d\n", ret); return 1;}
	else path_put(&src_path);
	printk( KERN_ALERT "swap_driver: SRC_NAME: %s, INODE: %ld\n", src_path.dentry->d_name.name, src_path.dentry->d_inode->i_ino);

	ret = kern_path(des_name, LOOKUP_FOLLOW, &des_path);
	if (ret) {pr_err("swap_driver: Failed to look up target directory, err:%d\n", ret); return 1;}
	else path_put(&des_path);
	printk( KERN_ALERT "swap_driver: DES_NAME: %s, INODE: %ld\n", des_path.dentry->d_name.name, des_path.dentry->d_inode->i_ino);

	SWAP(src_path.dentry->d_inode, des_path.dentry->d_inode, temp);
	printk( KERN_ALERT "swap_driver: FILE SWAPPED\n");
	return 0;
}
