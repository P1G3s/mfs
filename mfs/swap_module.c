#include "swap_module.h"

#define strlength(x) (sizeof(x) / sizeof((x)[0]))
#define SRCPATH "/home/p1g3s/workspace/mfs/TEMP/123"
#define DESPATH "/home/p1g3s/workspace/mfs/TEMP/456"

int ino_swap(){
	struct path src_path;
	struct path des_path;
	int ret = 0;

	ret = kern_path(SRCPATH, LOOKUP_FOLLOW, &src_path);
	if (ret) {pr_err("swap_driver: Failed to look up source directory, err:%d\n", ret); return 1;}
	else path_put(&src_path);
	printk( KERN_ALERT "swap_driver: SRC_NAME: %s, INODE: %ld\n", src_path.dentry->d_name.name, src_path.dentry->d_inode->i_ino);

	ret = kern_path(DESPATH, LOOKUP_FOLLOW, &des_path);
	if (ret) {pr_err("swap_driver: Failed to look up target directory, err:%d\n", ret); return 1;}
	else path_put(&des_path);
	printk( KERN_ALERT "swap_driver: DES_NAME: %s, INODE: %ld\n", des_path.dentry->d_name.name, des_path.dentry->d_inode->i_ino);


	printk( KERN_ALERT "swap_driver:");

	return 0;
}
