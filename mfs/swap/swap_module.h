#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/list.h>
#include <linux/slab.h>

typedef struct inode inode_t;
typedef inode_t** inode_list;



int ino_swap(const char*, const char*);
void ino_alloc(inode_t**, inode_t**);
void ino_recover(void);
void ino_init(void);
