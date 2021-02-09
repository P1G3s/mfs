#include "hide_module.h"
#include "hide_op.c"

static int hidden_dentry_count;
static struct dentry** hidden_dentry_list;
static struct file_operations** old_fop;
static struct inode_operations** old_iop;


int hide_ino_init(){
	int ret = 0;
	
	// LIST
	hidden_dentry_list = (struct dentry**) kmalloc(sizeof(void*), GFP_KERNEL);
	old_fop = (struct file_operations**) kmalloc(sizeof(void*), GFP_KERNEL);	
	old_iop = (struct inode_operations**) kmalloc(sizeof(void*), GFP_KERNEL);
	hidden_dentry_count = 0;
	if (!(hidden_dentry_list && old_fop && old_iop)){
		printk(KERN_WARNING "HIDE_DRIVER: Failed to initialize the inode list\n");
		ret = 1;
	}
	return ret;
}

int hide_ino_alloc(struct dentry* hidden_dentry){
	hidden_dentry_list[hidden_dentry_count] = hidden_dentry;
	old_fop[hidden_dentry_count] = hidden_dentry->d_inode->i_fop;
	old_iop[hidden_dentry_count] = hidden_dentry->d_inode->i_op;
	hidden_dentry_count += 1;
	
	printk(KERN_NOTICE "HIDE_DRIVER: Count = %d\n", hidden_dentry_count);
	hidden_dentry_list = (struct dentry**) krealloc(hidden_dentry_list, sizeof(void*) * (hidden_dentry_count+1), GFP_KERNEL);
	old_fop = (struct file_operations**) krealloc(old_fop, sizeof(void*) * (hidden_dentry_count+1), GFP_KERNEL);
	old_iop = (struct inode_operations**) krealloc(old_iop, sizeof(void*) * (hidden_dentry_count+1), GFP_KERNEL);
		
	return 0;
}

int hide_ino_hide(const char* path){
	struct path hidden_path;
	struct dentry* hidden_dentry;
	int ret;
	int val = 1;

	ret = kern_path(path, LOOKUP_FOLLOW, &hidden_path);
	if (ret) {ret = kern_path(path, LOOKUP_DIRECTORY, &hidden_path);}
	if (ret) {pr_err("HIDE_DRIVER: Failed to look up target, err:%d\n", ret);return 1;}
	else path_put(&hidden_path);	

	hidden_dentry = hidden_path.dentry;
	printk(KERN_ALERT "HIDE_DRIVER: Hiding '%s'\n", hidden_dentry->d_name.name);
	if (hide_ino_alloc(hidden_dentry)) return 1;
	vfs_setxattr(hidden_dentry, "user.mfs_delete", &val, sizeof(int), 0);	
//	hidden_dentry->d_inode->i_op = &(new_iop);
//	hidden_dentry->d_inode->i_fop = &(new_fop);
	return 0;
}

int hide_ino_recover(){
	int i = 0;
	int val = 0;
	struct dentry* cur_dentry;
	printk(KERN_ALERT "HIDE_DRIVER: ### Recovering ###\n");
	while (i < hidden_dentry_count){
		cur_dentry = hidden_dentry_list[i];
		cur_dentry->d_inode->i_fop = old_fop[i];
		cur_dentry->d_inode->i_op = old_iop[i];
		printk(KERN_NOTICE "HIDE_DRIVER: %d. %s\n", i+1, cur_dentry->d_name.name);
		vfs_setxattr(cur_dentry, "user.mfs_delete", &val, sizeof(int), 0); //SAFETY?
		vfs_removexattr(cur_dentry, "user.mfs_delete");
		i++;
	}
	printk(KERN_ALERT "HIDE_DRIVER: ### Done ###\n");
	kfree(hidden_dentry_list);
	kfree(old_fop);
	kfree(old_iop);
	hide_ino_init();
	return 0;
}
