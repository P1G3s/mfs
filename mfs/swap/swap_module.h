#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/list.h>
#include <linux/slab.h>

typedef struct inode inode_t;

int ino_swap(const char* src_name, const char* des_name);
