#include "swap_module.h"

#define SWAP(x,y,temp) {temp=x; x=y; y=temp;}

static dentry_t** org_dentry_list;
static dentry_t** new_dentry_list;
// static unsigned long* ino_list; // FOR CHECKING IF INDOES ARE SWAPPED
static int swapped_dentry_count;

// INIT THE LIST
void ino_init(){
	org_dentry_list = (dentry_t**) kmalloc(sizeof(void*), GFP_KERNEL);
	new_dentry_list = (dentry_t**) kmalloc(sizeof(void*), GFP_KERNEL);
	swapped_dentry_count = 0;
	if (!(org_dentry_list && new_dentry_list)) {
		printk(KERN_WARNING "SWAP_DRIVER: Failed to initialize the dentry list\n");
	}
}

// STORE THE SWAPPED INODES
void ino_alloc(dentry_t* src_dentry, dentry_t* des_dentry){
	org_dentry_list[swapped_dentry_count] = src_dentry;
	new_dentry_list[swapped_dentry_count] = des_dentry;
	swapped_dentry_count += 1;
	printk(KERN_NOTICE "SWAP_DRIVER: Count = %d\n", swapped_dentry_count);
	
	// REALLOCATE THE LIST
	org_dentry_list = (dentry_t **) krealloc(org_dentry_list, sizeof(void*) * (swapped_dentry_count+1), GFP_KERNEL);
	new_dentry_list = (dentry_t **) krealloc(new_dentry_list, sizeof(void*) * (swapped_dentry_count+1), GFP_KERNEL);
}

// RECOVER THE SWAPPED FILES BEFORE UNLOAD
void ino_recover(){
	dentry_t* src_dentry;
	dentry_t* des_dentry;
	inode_t* temp;
	int val = 0;
	int i = 0;
	printk(KERN_ALERT "SWAP_DRIVER: ### Recovering ###\n");
	while(i < swapped_dentry_count){
		src_dentry = org_dentry_list[i];
		des_dentry = new_dentry_list[i];
		SWAP(src_dentry->d_inode, des_dentry->d_inode, temp);
		vfs_setxattr(src_dentry, "user.mfs_swap", &val, sizeof(int), 0);
		vfs_setxattr(des_dentry, "user.mfs_swap", &val, sizeof(int), 0);
		vfs_removexattr(src_dentry, "user.mfs_swap");
		vfs_removexattr(des_dentry, "user.mfs_swap");
		printk(KERN_NOTICE "SWAP_DRIVER: %d. '%ld' <-> '%ld'\n", i, src_dentry->d_inode->i_ino, des_dentry->d_inode->i_ino); 
		i++;
	}
	kfree(org_dentry_list);
	kfree(new_dentry_list);
	printk(KERN_ALERT "SWAP_DRIVER: ### Done ###\n");
	ino_init();
}

// SWAP FILES' INODES
int ino_swap(const char* src_name, const char* des_name){
	struct path src_path;
	struct path des_path;
	struct file* src_filp;
	struct file* des_filp;
	dentry_t* src_dentry; 
	dentry_t* des_dentry;
	inode_t* temp;
	int ret = 0;
	int val = 1;
	
	// DENTRY'S INODE ACQUISITION
	ret = kern_path(src_name, LOOKUP_FOLLOW, &src_path);
	printk(KERN_ALERT "SWAP_DRIVER: Swapping '%s' to '%s'\n", src_name, des_name);
	if (ret) {pr_err("SWAP_DRIVER: Failed to look up source directory, err:%d\n", ret); return 1;}
	else path_put(&src_path);
	src_dentry = src_path.dentry;
	vfs_setxattr(src_dentry, "user.mfs_swap", &val, sizeof(int), 0);

	ret = kern_path(des_name, LOOKUP_FOLLOW, &des_path);
	if (ret) {pr_err("SWAP_DRIVER: Failed to look up target directory, err:%d\n", ret); return 1;}
	else path_put(&des_path);
	des_dentry = des_path.dentry;
	vfs_setxattr(des_dentry, "user.mfs_swap", &val, sizeof(int), 0);

	ino_alloc(src_dentry, des_dentry);
	SWAP(src_dentry->d_inode, des_dentry->d_inode, temp);

	// FILE'S INODE ACQUISITION
	temp = NULL;
	src_filp = filp_open(src_name, O_RDONLY, 0);
	des_filp = filp_open(des_name, O_RDONLY, 0);
	SWAP(src_filp->f_inode, des_filp->f_inode, temp);
	filp_close(src_filp, 0);
	filp_close(des_filp, 0);

	return 0;
}

