#define FUSE_USE_VERSION 31

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE
#define MAXLEN 256

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#include <sys/types.h>
#include <sys/xattr.h>
#include <stdlib.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#ifdef __FreeBSD__
#include <sys/socket.h>
#include <sys/un.h>
#endif
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#include "mfs_helpers.h"

#define TEMPDIR "/home/P1G3s/WorkSpace/MFS/mfs/TEMP/"

static int log_fd;
static int hide_fd;
static int swap_fd;
static int temp_count;

static void mfs_log(const char* msg){
	write(log_fd, msg, strlen(msg));
}

static void *mfs_init(struct fuse_conn_info *conn,
		      struct fuse_config *cfg)
{
	(void) conn;
	cfg->use_ino = 1;

	/* Pick up changes from lower filesystem right away. This is
	   also necessary for better hardlink support. When the kernel
	   calls the unlink() handler, it does not know the inode of
	   the to-be-removed entry and can therefore not invalidate
	   the cache of the associated inode - resulting in an
	   incorrect st_nlink value being reported for any remaining
	   hardlinks to this inode. */
	cfg->entry_timeout = 0;
	cfg->attr_timeout = 0;
	cfg->negative_timeout = 0;

	return NULL;
}

static int mfs_getattr(const char *path, struct stat *stbuf,
		       struct fuse_file_info *fi)
{
	(void) fi;
	int res;

	res = lstat(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int mfs_access(const char *path, int mask)
{
	int res;

	res = access(path, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int mfs_readlink(const char *path, char *buf, size_t size)
{
	int res;

	res = readlink(path, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int mfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi,
		       enum fuse_readdir_flags flags)
{
	DIR *dp;
	struct dirent *de;
	char file_path[MAXLEN];
	int attr_value;
	int res;

	(void) offset;
	(void) fi;
	(void) flags;

	dp = opendir(path);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		strcpy(file_path, path);	
		strcat(file_path, "/");
		strcat(file_path, de->d_name);

		// CHECK FOR DELETED FILE
		res = getxattr(file_path, "user.mfs_delete", &attr_value, sizeof(int));
		if (res != ENODATA){
			if (attr_value == 1){
				mfs_log(file_path);
				mfs_log(" -> gone\n");
				attr_value = 0;
				continue;
			}
		}

		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int mfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;

	res = mknod_wrapper(AT_FDCWD, path, NULL, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int mfs_mkdir(const char *path, mode_t mode)
{
	int res;

	res = mkdir(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int mfs_unlink(const char *path)
{
	//mfs_log(path);
	//mfs_log(" -> unlinked\n");
	char* buf = (char*) malloc((strlen(path)+1)*sizeof(char));
	strcpy(buf+sizeof(char),path);
	buf[0] = 'H';
	if (write(hide_fd, buf, strlen(buf)) == -1)
		return -1;
	free(buf);
	return 0;
}

static int mfs_rmdir(const char *path)
{

	//mfs_log(path);
	//mfs_log(" -> removed\n");
	char* buf = (char*) malloc((strlen(path)+1)*sizeof(char));
	strcpy(buf+sizeof(char),path);
	buf[0] = 'H';
	if (write(hide_fd, buf, strlen(buf)) == -1)
		return -1;
	free(buf);
	return 0;
}

static int mfs_symlink(const char *from, const char *to)
{
	int res;

	res = symlink(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int mfs_rename(const char *from, const char *to, unsigned int flags)
{
	int res;

	if (flags)
		return -EINVAL;

	res = rename(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int mfs_link(const char *from, const char *to)
{
	int res;

	res = link(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int mfs_chmod(const char *path, mode_t mode,
		     struct fuse_file_info *fi)
{
	(void) fi;
	int res;

	res = chmod(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int mfs_chown(const char *path, uid_t uid, gid_t gid,
		     struct fuse_file_info *fi)
{
	(void) fi;
	int res;

	res = lchown(path, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int mfs_truncate(const char *path, off_t size,
			struct fuse_file_info *fi)
{
	int res;

	if (fi != NULL)
		res = ftruncate(fi->fh, size);
	else
		res = truncate(path, size);
	if (res == -1)
		return -errno;

	return 0;
}

#ifdef HAVE_UTIMENSAT
static int mfs_utimens(const char *path, const struct timespec ts[2],
		       struct fuse_file_info *fi)
{
	(void) fi;
	int res;

	/* don't use utime/utimes since they follow symlinks */
	res = utimensat(0, path, ts, AT_SYMLINK_NOFOLLOW);
	if (res == -1)
		return -errno;

	return 0;
}
#endif

static int mfs_create(const char *path, mode_t mode,
		      struct fuse_file_info *fi)
{
	int res;

	res = open(path, fi->flags, mode);
	if (res == -1)
		return -errno;

	fi->fh = res;
	return 0;
}

static int mfs_open(const char *path, struct fuse_file_info *fi)
{
	int res;

	res = open(path, fi->flags);
	if (res == -1)
		return -errno;

	fi->fh = res;
	return 0;
}

static int mfs_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int fd;
	int res;

	if(fi == NULL)
		fd = open(path, O_RDONLY);
	else
		fd = fi->fh;
	
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	if(fi == NULL)
		close(fd);
	return res;
}

static int mfs_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;
	int ret;
	int val;

	(void) fi;
	ret = getxattr(path, "user.mfs_swap", &val, sizeof(int));
	if (ret == ENODATA || val == 0){
		int len, ret; 
		int write_len;
		char* write_val;
		char* temp_path;
		int temp_fd, src_fd;
		struct stat src_stat;

		temp_path = (char*) malloc(sizeof(char)*(strlen(TEMPDIR)+10));
		if (sprintf(temp_path, "%s%d", TEMPDIR, temp_count) < 0) {mfs_log("Failed to create path\n"); return -2;}
		temp_count += 1;

		// COPY
		temp_fd = open(temp_path, O_RDWR | O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
		if (temp_fd == -1) return -errno;
		src_fd = open(path, O_RDWR);
		if (fstat(src_fd, &src_stat) == -1) return -errno;
		len = src_stat.st_size;
		do{
			ret = copy_file_range(src_fd, NULL, temp_fd, NULL, len, 0);
			if (ret == -1) return -errno;
			len -= ret;
		}while (len > 0);
		close(temp_fd); close(src_fd);

		// SWAP
		write_len = strlen(path)+strlen(temp_path)+2;
		write_val = (char*) malloc(sizeof(char)* write_len);
		write_val[0] = 'S';
		strcpy(write_val+1, path);
		strcat(write_val, " ");
		strcat(write_val, temp_path);
		if (write(swap_fd, write_val, write_len) == -1) {mfs_log("Failed to swap\n"); return -2;}
		
		free(write_val); free(temp_path);
	}
	/*
	if (fi == NULL)
		fd = open(path, O_WRONLY);
	else
		fd = fi->fh;
	*/
	fd = open(path, O_WRONLY);
	
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	if(fi == NULL)
		close(fd);
	return res;
}

static int mfs_statfs(const char *path, struct statvfs *stbuf)
{
	int res;

	res = statvfs(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int mfs_release(const char *path, struct fuse_file_info *fi)
{
	(void) path;
	close(fi->fh);
	return 0;
}

static int mfs_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	(void) path;
	close(fi->fh);
	return 0;
}

#ifdef HAVE_POSIX_FALLOCATE
static int mfs_fallocate(const char *path, int mode,
			off_t offset, off_t length, struct fuse_file_info *fi)
{
	int fd;
	int res;

	(void) fi;

	if (mode)
		return -EOPNOTSUPP;

	if(fi == NULL)
		fd = open(path, O_WRONLY);
	else
		fd = fi->fh;
	
	if (fd == -1)
		return -errno;

	res = -posix_fallocate(fd, offset, length);

	if(fi == NULL)
		close(fd);
	return res;
}
#endif

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int mfs_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	int res = setxattr(path, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int mfs_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	int res = lgetxattr(path, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int mfs_listxattr(const char *path, char *list, size_t size)
{
	int res = llistxattr(path, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int mfs_removexattr(const char *path, const char *name)
{
	int res = lremovexattr(path, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

#ifdef HAVE_COPY_FILE_RANGE
static ssize_t mfs_copy_file_range(const char *path_in,
				   struct fuse_file_info *fi_in,
				   off_t offset_in, const char *path_out,
				   struct fuse_file_info *fi_out,
				   off_t offset_out, size_t len, int flags)
{
	int fd_in, fd_out;
	ssize_t res;

	if(fi_in == NULL)
		fd_in = open(path_in, O_RDONLY);
	else
		fd_in = fi_in->fh;

	if (fd_in == -1)
		return -errno;

	if(fi_out == NULL)
		fd_out = open(path_out, O_WRONLY);
	else
		fd_out = fi_out->fh;

	if (fd_out == -1) {
		close(fd_in);
		return -errno;
	}

	res = copy_file_range(fd_in, &offset_in, fd_out, &offset_out, len,
			      flags);
	if (res == -1)
		res = -errno;

	if (fi_out == NULL)
		close(fd_out);
	if (fi_in == NULL)
		close(fd_in);

	return res;
}
#endif

static off_t mfs_lseek(const char *path, off_t off, int whence, struct fuse_file_info *fi)
{
	int fd;
	off_t res;

	if (fi == NULL)
		fd = open(path, O_RDONLY);
	else
		fd = fi->fh;

	if (fd == -1)
		return -errno;

	res = lseek(fd, off, whence);
	if (res == -1)
		res = -errno;

	if (fi == NULL)
		close(fd);
	return res;
}

static const struct fuse_operations mfs_oper = {
	.init           = mfs_init,
	.getattr	= mfs_getattr,
	.access		= mfs_access,
	.readlink	= mfs_readlink,
	.readdir	= mfs_readdir,
	.mknod		= mfs_mknod,
	.mkdir		= mfs_mkdir,
	.symlink	= mfs_symlink,
	.unlink		= mfs_unlink,
	.rmdir		= mfs_rmdir,
	.rename		= mfs_rename,
	.link		= mfs_link,
	.chmod		= mfs_chmod,
	.chown		= mfs_chown,
	.truncate	= mfs_truncate,
#ifdef HAVE_UTIMENSAT
	.utimens	= mfs_utimens,
#endif
	.open		= mfs_open,
	.create 	= mfs_create,
	.read		= mfs_read,
	.write		= mfs_write,
	.statfs		= mfs_statfs,
	.release	= mfs_release,
	.fsync		= mfs_fsync,
#ifdef HAVE_POSIX_FALLOCATE
	.copy_file_range = mfs_copy_file_range,
#endif
	.lseek		= mfs_lseek,
};

int main(int argc, char *argv[])
{
	umask(0);

	// INIT STUFF
	temp_count = 0;
	fclose(fopen("mfs.log", "w"));
	log_fd = open("./mfs.log", O_CREAT | O_RDWR | O_APPEND);
	hide_fd = open("/dev/hide_device", O_RDWR);
	swap_fd = open("/dev/swap_device", O_RDWR);
	if ((swap_fd == -1) || (log_fd == -1) || (hide_fd == -1)){
		printf("Failed to open logs or device\n");
		return -1;
	}
	fuse_main(argc, argv, &mfs_oper, NULL);
	close(log_fd);
	close(hide_fd);
	close(swap_fd);
	return 0;
}
