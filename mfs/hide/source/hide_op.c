#include <linux/errno.h>

// FOP
int new_mmap (struct file * file, struct vm_area_struct * area)
{
	//printk( KERN_ALERT "Entered in new_mmap\n");
	return -2;
}

ssize_t new_read (struct file *file1, char __user * u, size_t t, loff_t *ll)
{
	//printk( KERN_ALERT "Entered in new_read\n");
	return -2;
}

ssize_t new_write (struct file * file1, const char __user * u, size_t t, loff_t *ll)
{
	//printk( KERN_ALERT "Entered in new_write\n");
	return -2;
}

int new_release (struct inode * new_inode, struct file *file)
{
	//printk( KERN_ALERT "Entered in new_release \n");
	return -2;
}

int new_flush (struct file *file, fl_owner_t id)
{
	//printk( KERN_ALERT "Entered in new_flush \n");
	return -2;
}

int new_readdir (struct file *file, void *dirent, filldir_t filldir)
{
	//printk( KERN_ALERT "Entered in new_readdir \n");
	return -2;
}

int new_open (struct inode * old_inode, struct file * old_file)
{
	//printk( KERN_ALERT "Entered in new_open \n");
	return -2;
}

static struct file_operations new_fop =
{
	.owner=THIS_MODULE,
	.release=new_release,
	.open=new_open,
	.read=new_read, 
	.write=new_write,
	.mmap=new_mmap,
};


//IOP
int new_rmdir (struct inode *new_inode,struct dentry *new_dentry)
{
	//printk( KERN_ALERT "Entered in new_rmdir \n");
	return -2;
}

int new_getattr (struct vfsmount *mnt, struct dentry * new_dentry, struct kstat * ks)
{
	//printk( KERN_ALERT "Entered in new_getatr \n");
	return -2;
}

static struct inode_operations new_iop =
{
	//.getattr=new_getattr,
	.rmdir=new_rmdir,
};
