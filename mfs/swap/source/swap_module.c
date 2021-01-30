#include "swap_module.h"

#define SWAP(x,y,temp) {temp=x; x=y; y=temp;}

static inode_list* org_inode_list;
static inode_list* new_inode_list;
static unsigned long* ino_list;
static int inode_count = 0;

// INIT THE LIST
void ino_init(){
	org_inode_list = (inode_list*) kmalloc(sizeof(void*), GFP_KERNEL);
	new_inode_list = (inode_list*) kmalloc(sizeof(void*), GFP_KERNEL);
	ino_list = (unsigned long*) kmalloc(sizeof(void*), GFP_KERNEL);
	if (!(org_inode_list && new_inode_list && ino_list)) {
		printk(KERN_ALERT "SWAP_DRIVER: Failed to initialize the inode list");
	}
}

// STORE THE SWAPPED INODES
void ino_alloc(inode_t** src_inode, inode_t** des_inode){
	org_inode_list[inode_count] = src_inode;
	new_inode_list[inode_count] = des_inode;
	ino_list[inode_count] = (*des_inode)->i_ino;
	printk(KERN_ALERT "SWAP_DRIVER: Allocating for %ld and %ld\n",
			(*org_inode_list[inode_count])->i_ino,
			(*new_inode_list[inode_count])->i_ino);
	inode_count += 1;
	printk(KERN_ALERT "SWAP_DRIVER: Count = %d\n", inode_count);
	
	// REALLOCATE THE LIST
	org_inode_list = (inode_list*) krealloc(org_inode_list, sizeof(void*) * (inode_count+1), GFP_KERNEL);
	new_inode_list = (inode_list*) krealloc(new_inode_list, sizeof(void*) * (inode_count+1), GFP_KERNEL);
	ino_list = (unsigned long*) krealloc(ino_list, sizeof(void*) * (inode_count+1), GFP_KERNEL);
}

// RECOVER THE SWAPPED FILES BEFORE UNLOAD
void ino_recover(){
	inode_t* temp;
	int i = 0;
	while(i < inode_count){
		printk(KERN_ALERT "SWAP_DRIVER: Recovering from %ld to %ld\n", 
				(*org_inode_list[i])->i_ino,
				(*new_inode_list[i])->i_ino);
		SWAP(*(org_inode_list[i]), *(new_inode_list[i]), temp);
		i++;
	}
}

// SWAP FILES' INODES
int ino_swap(const char* src_name, const char* des_name){
	struct path src_path;
	struct path des_path;
	inode_t** src_inode; // WHICH HOLDS THE ADDRESS OF 'd_inode'
	inode_t** des_inode;
	inode_t* temp;
	int ret = 0;
	
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

	ino_alloc(src_inode, des_inode);
	SWAP((*src_inode), (*des_inode), temp);
	printk(KERN_ALERT "SWAP_DRIVER: FILE SWAPPED\n");
	return 0;
}

// CHECK IF FILE IS ALREADY SWAPPED
int ino_is_swapped(const char* file_name){
	struct path file_path;
	unsigned long cur_ino;
	int ret = 0;
	int i = 0;

	ret = kern_path(file_name, LOOKUP_FOLLOW, &file_path);
	if (ret) {pr_err("SWAP_DRIVER: Failed to look up source directory, err:%d\n", ret); return -1;}
	else path_put(&file_path);
	cur_ino = file_path.dentry->d_inode->i_ino; 
	while (i < inode_count){
		if (cur_ino == ino_list[i]){
			ret = 1;
			break;
		}	
		i++;
	}
	return ret;
}
