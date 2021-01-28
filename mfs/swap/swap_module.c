#include "swap_module.h"

#define SWAP(x,y,temp) {temp=x; x=y; y=temp;}

static inode_t** org_inode_list;
static inode_t** new_inode_list;
static int inode_count = 0;

/*
// INIT THE LIST
void ino_init(){
	int ret = 0;
	org_inode_list = (inode_t**) kmalloc(sizeof(void*), GFP_KERNEL);
	new_inode_list = (inode_t**) kmalloc(sizeof(void*), GFP_KERNEL);
	if (!(src_inode && des_inode)) {
		printk(KERN_ALERT,"SWAP_DRIVER: Failed to initialize the inode list");
		ret = -1;
	}
	return ret;
}

// STORE THE SWAPPED INODES
void ino_alloc(inode_t* src_inode, inode_t* des_inode){
	org_inode_list[inode_count] = src_inode;
	new_inode_list[inode_count] = des_inode;
	inode_count += 1;
	// REALLOCATE THE LIST
	org_inode_list = (inode_t**) krealloc(org_inode_list, sizeof(void*) * (inode_count), GFP_KERNEL);
	new_inode_list = (inode_t**) krealloc(org_inode_list, sizeof(void*) * (inode_count), GFP_KERNEL);
}

void ino_recover(){
	inode_t* temp;
	inode_t src_inode;
	inode_t des_inode;
	for(int i=0; i<inode_count; i++){
	}
}
*/

int ino_swap(const char* src_name, const char* des_name){
	struct path src_path;
	struct path des_path;
	inode_t** src_inode; // WHICH HOLDS THE ADDRESS OF 'd_inode'
	inode_t** des_inode;
	inode_t* temp;
	int ret = 0;

	// INIT
	src_inode = (inode_t**) kmalloc(sizeof(void**), GFP_KERNEL);
	des_inode = (inode_t**) kmalloc(sizeof(void**), GFP_KERNEL);
	
	// INODE ACQUISITION
	ret = kern_path(src_name, LOOKUP_FOLLOW, &src_path);
	if (ret) {pr_err("SWAP_DRIVER: Failed to look up source directory, err:%d\n", ret); return 1;}
	else path_put(&src_path);
	src_inode = &(src_path.dentry->d_inode);
	printk(KERN_ALERT "SWAP_DRIVER: SRC_NAME: %s, INODE: %ld\n", src_path.dentry->d_name.name, (*src_inode)->i_ino);

	ret = kern_path(des_name, LOOKUP_FOLLOW, &des_path);
	if (ret) {pr_err("SWAP_DRIVER: Failed to look up target directory, err:%d\n", ret); return 1;}
	else path_put(&des_path);
	des_inode = &(des_path.dentry->d_inode);
	printk(KERN_ALERT "SWAP_DRIVER: DES_NAME: %s, INODE: %ld\n", des_path.dentry->d_name.name, (*des_inode)->i_ino);

	//SWAP((*src_inode), (*des_inode), temp);
	temp = *src_inode;	
	*src_inode = *des_inode;
	*des_inode = temp;
	printk(KERN_ALERT "SWAP_DRIVER: FILE SWAPPED\n");
	kfree(src_inode);
	kfree(des_inode);
	return 0;
}
