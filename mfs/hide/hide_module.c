#include "hide_module.h"
#include "hide_child_op.c"

static int inode_count;
static inode_list child_inode_list;
static inode_list parent_inode_list;
static struct file_operations* old_child_fop;
static struct file_operations* old_parent_fop;
static struct inode_operations* old_child_iop;
static unsigned long* hidden_inode_list;

filldir_t real_filldir;


// HIDE_PARENT_OP
static struct path root_path;

static struct file_operations new_parent_fop ={
	.owner=THIS_MODULE,
	.readdir=parent_readdir,
};

static int new_filldir (void *buf, const char *name, int namelen, loff_t offset,u64 ux64, unsigned ino){
	int i=0;
	int ret = 0;
	struct path current_path;

	ret = kern_path(name, LOOKUP_FOLLOW, &current_path);
	if (ret) {pr_err("HIDE_DRIVER: Failed to look up source directory, err:%d\n", ret); return 1;}
	else path_put(&current_path);

	// #DEREFERENCE NEEDED#
	while(i < inode_count){
		if (hidden_inode_list[i]==current_path.dentry->d_inode->i_ino){
			printk(KERN_ALERT "HIDE_DRIVER: File - '%s' is hidden\n", name);
			return 0;
		}
		i++;
	}
	return real_filldir (buf, name, namelen, offset, ux64, ino);
}

int parent_readdir (struct file *file, void *dirent, filldir_t filldir){	
	// g_parent_dentry = file->f_dentry;
	real_filldir = filldir;
	return root_path.dentry->d_inode->i_fop->readdir(file, dirent, new_filldir);
}



int hide_ino_init(){
	int ret;
	// LIST
	child_inode_list = (inode_list*) kmalloc(sizeof(void*), GFP_KERNEL);
	parent_inode_list = (inode_list*) kmalloc(sizeof(void*), GFP_KERNEL);
	old_child_fop = (file_operations*) kmalloc(sizeof(void*), GFP_KERNEL);	
	old_child_iop = (inode_operations*) kmalloc(sizeof(void*), GFP_KERNEL);
	inode_count = 0;
	if (!(parent_inode_list && child_inode_list))
		printk(KERN_WARNING "HIDE_DRIVER: Failed to initialize the inode list\n");
	
	// ROOT_PATH
	ret = kern_path("/", LOOKUP_FOLLOW, &root_path);
	if (ret) {pr_err("HIDE_DRIVER: Failed to look up source directory, err:%d\n", ret); return 1;}
	else path_put(&root_path);
	return ret
}

int hide_ino_alloc(inode_t** child_inode, inode_t** parent_inode){
	child_inode_list[inode_count] = child_inode;
	parent_inode_list[inode_count] = parent_inode;
	old_child_fop[inode_count] = (*child_inode)->i_fop;
	old_child_iop[inode_count] = (*child_inode)->i_op;
	old_parent_fop[inode_count] = (*parent_inode)->i_fop;
	inode_count += 1;
	
	printk(KERN_NOTICE "HIDE_DRIVER: Count = %d\n", inode_count);
	child_inode_list = (inode_t**) krealloc(child_inode_list, sizeof(void*) * (inode_count+1), GFP_KERNEL);
	parent_inode_list = (inode_t**) krealloc(parent_inode_list, sizeof(void*) * (inode_count+1), GFP_KERNEL);
	hidden_inode_list = (unsigned long*) krealloc(hidden_inode_list, sizeof(void*) * (inode_count+1), GFP_KERNEL);
	
	return 0;
}

int hide_ino_hide(const char* file_name){ 
	struct file_path;
	inode_t** child_inode;
	inode_t** parent_inode;
	int ret = 0;	

	// INODE ACQUISITION
	printk( KERN_ALERT "HIDE_DRIVER: Hidding file '%s'\n", file_name);
	ret = kern_path(file_name, LOOKUP_FOLLOW, &file_path);
	if (ret) {pr_err("SWAP_DRIVER: Failed to look up source directory, err:%d\n", ret); return 1;}
	else path_put(&file_path);

	// SAVE OLD OP
	hide_ino_alloc(child_inode, parent_inode);

	// HOOK NEW CHILD FOP/IOP
	child_inode = &(file_path.dentry->d_inode);
	(*child_inode)->i_op = new_child_iop;
	(*child_inode)->i_fop = new_child_fop;
	
	// HOOK NEW PARENT FOP
	parent_inode = &(file_path.dentry->d_parent->d_inode);	
	(*parent_inode)->i_fop = new_parent_fop;

	return ret;
}

int hide_ino_recover(){
	return 0;
}
