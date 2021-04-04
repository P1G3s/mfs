#define FUSE_USE_VERSION 31

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE
#define MAXLEN 1024

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#include <sys/types.h>
#include <stdlib.h>
#include <sys/xattr.h>
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

#define TEMPDIR "/home/p1g3s/Downloads/mfs/mfs/TEMP/"

static int log_fd;
static int hide_fd;
static int swap_fd;
static int rm_fd;
static int chmod_fd;

static int temp_count;
static int mode_count;
static mode_t* mode_list;

static void mfs_log(const char* msg){
	write(log_fd, msg, strlen(msg));
}

static void mfs_rm_log(const char* msg){
	write(rm_fd, msg, strlen(msg));
	write(rm_fd, "\n", 1);
}

static void mfs_chmod_log(const char* msg){
	write(chmod_fd, msg, strlen(msg));
	write(chmod_fd, "\n", 1);
}

static int copy_to_temp(const char* src_path, const char* temp_path){
	int src_fd, temp_fd;
	int ret, len;
	struct stat src_stat;
	
	temp_fd = open(temp_path, O_RDWR | O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
	if (temp_fd == -1) {perror("copy_to_temp(open)"); return -errno;}
	src_fd = open(src_path, O_RDWR);
	if (fstat(src_fd, &src_stat) == -1) {perror("copy_to_temp(fstat)"); return -errno;}
	len = src_stat.st_size;
	do{
		ret = copy_file_range(src_fd, NULL, temp_fd, NULL, len, 0);
		if (ret == -1) {perror("copy_to_temp(copy)"); return -errno;}
		len -= ret;
	}while (len > 0);
	close(src_fd); close(temp_fd);
	return 0;
}

static void mfs_rm_recover(){
	int mark_arr[1000];	
	int i = -1;
	int j = 0;
	int len;
	char mark;
	char buf[MAXLEN];

	lseek(rm_fd, 0, SEEK_SET);
	while (read(rm_fd, &mark, 1) == 1){
		if (mark == '\n'){
			i++;
			mark_arr[i] = j;
		}
		j++;
	}
	if (i == -1) return;

	while (i>0){
		len = mark_arr[i]-mark_arr[i-1]-1;
		lseek(rm_fd, mark_arr[i-1]+1, SEEK_SET);
		read(rm_fd, buf, len);
		buf[len] = '\0';
		if (unlink(buf) == -1 && errno == EISDIR)
			rmdir(buf);
		i--;
	}
	// ONE MORE
	len = mark_arr[0];
	lseek(rm_fd, 0, SEEK_SET);
	read(rm_fd, buf, len);
	buf[len] = '\0';
	if (unlink(buf) == -1 && errno == EISDIR)
		rmdir(buf);
}

static void mfs_chmod_recover(){
	int mark_arr[1000];	// LAZY ALLOCATION :(
	int i = -1;
	int j = 0;
	int len;
	char mark;
	char buf[MAXLEN];

	lseek(chmod_fd, 0, SEEK_SET);
	while (read(chmod_fd, &mark, 1) == 1){
		if (mark == '\n'){
			i++;
			mark_arr[i] = j;
		}
		j++;
	}
	if (i == -1) return;

	while (i>0){
		len = mark_arr[i]-mark_arr[i-1]-1;
		lseek(chmod_fd, mark_arr[i-1]+1, SEEK_SET);
		read(chmod_fd, buf, len);
		buf[len] = '\0';
		chmod(buf, mode_list[i]);
		i--;
	}
	// ONE MORE
	len = mark_arr[0];
	lseek(chmod_fd, 0, SEEK_SET);
	read(chmod_fd, buf, len);
	buf[len] = '\0';
	chmod(buf, mode_list[0]);
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
	char val;
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
		res = getxattr(file_path, "user.mfs_delete", &val, sizeof(char));
		if (res != ENODATA){
			if (val == '1'){
				mfs_log(file_path);
				mfs_log(" -> hidden\n");
				val = '0';
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
	int val = 1;

	res = mkdir(path, mode);
	setxattr(path, "user.mfs_swap", &val, sizeof(int), 0);
	mfs_rm_log(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int mfs_unlink(const char *path)
{
	int val = 0;
	getxattr(path, "user.mfs_swap", &val, sizeof(int));
	// IF PATH DOESNT EXIST
	printf("unlink: %s\n", path);

	if(access(path, F_OK) != 0){ 
		return -ENOENT;
	}
	else if(val == 1){
		unlink(path);
		return 0;
	}
	else{
		char* buf = (char*) malloc((strlen(path)+1)*sizeof(char));
		strcpy(buf+sizeof(char),path);
		buf[0] = 'H';
		if (write(hide_fd, buf, strlen(buf)) == -1)
			return -1;
		free(buf);
		return 0;
	}
}

static int mfs_rmdir(const char *path)
{
	int val = 0;
	DIR* dir = opendir(path);
	getxattr(path, "user.mfs_swap", &val, sizeof(int));
	if (!dir){
		return -ENOENT;
	}
	else if(val == 1){
		rmdir(path);
		return 0;
	}
	else{
		char* buf = (char*) malloc((strlen(path)+1)*sizeof(char));
		strcpy(buf+sizeof(char),path);
		buf[0] = 'H';
		if (write(hide_fd, buf, strlen(buf)) == -1)
			return -1;
		free(buf);
		return 0;
	}
}

static int mfs_symlink(const char *from, const char *to)
{
	int res;
	int val = 1;

	res = symlink(from, to);
	setxattr(to, "user.mfs_swap", &val, sizeof(int), 0);
	mfs_rm_log(to);
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
	int val = 1;

	res = link(from, to);
	setxattr(to, "user.mfs_swap", &val, sizeof(int), 0);
	mfs_rm_log(to);

	if (res == -1)
		return -errno;

	return 0;
}

static int mfs_chmod(const char *path, mode_t mode,
		     struct fuse_file_info *fi)
{
	(void) fi;
	int res;
	struct stat path_stat;

	if (stat(path, &path_stat) < 0)    
        return 1;
	/*
	char* msg = (char*) malloc (sizeof(char)*(strlen(path)+10));
	if (sprintf(msg, "%s %d", path, path_stat.st_mode) < 0) {
		printf("chmod(sprintf)");
		return 0;
	}
	*/
	mode_list[mode_count] = path_stat.st_mode;	
	mode_count++;
	mode_list = (mode_t*) realloc(mode_list, sizeof(mode_t)*(mode_count+1));
	mfs_chmod_log(path);

	res = chmod(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

/*
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
*/

static int mfs_truncate(const char *path, off_t size,
			struct fuse_file_info *fi)
{
	int res;
	int val=0;

	getxattr(path, "user.mfs_swap", &val, sizeof(int));
	if (val == 0){ // ret == ENODATA?
		int write_len;
		char* write_val;
		char* temp_path;

		temp_path = (char*) malloc(sizeof(char)*(strlen(TEMPDIR)+10));
		if (sprintf(temp_path, "%s%d", TEMPDIR, temp_count) < 0) {
			printf("truncate(sprintf)\n");
			return 0;
		}
		temp_count += 1;

		// COPY
		if (copy_to_temp(path, temp_path) != 0)
			return -errno;

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

	res = truncate(path, size);
	if (fi != NULL)
		res = ftruncate(fi->fh, size);
	else
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
	int val = 1;

	res = open(path, fi->flags, mode);
	if (res == -1)
		return -errno;
	setxattr(path, "user.mfs_swap", &val, sizeof(int), 0);
	mfs_rm_log(path);

	fi->fh = res;
	return 0;
}

static int mfs_open(const char *path, struct fuse_file_info *fi)
{
	int res;
	int val = 1;

	printf("open: %s\n", path);
	res = open(path, fi->flags);
	
	if (res == -1)
		return -errno;
	if ((fi->flags & O_CREAT) == O_CREAT){
		setxattr(path, "user.mfs_swap", &val, sizeof(int), 0);
		mfs_rm_log(path);
	}

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
	int val=0;

	(void) fi;
	int ret = getxattr(path, "user.mfs_swap", &val, sizeof(int));
	if (val == 0 || ret == ENODATA){ // ret == ENODATA?
		int write_len;
		char* write_val;
		char* temp_path;

		temp_path = (char*) malloc(sizeof(char)*(strlen(TEMPDIR)+10));
		if (sprintf(temp_path, "%s%d", TEMPDIR, temp_count) < 0) {
			printf("write(sprintf)\n");
			return 0;
		}
		temp_count += 1;

		printf("%s\n", temp_path);
		// COPY
		if (copy_to_temp(path, temp_path) != 0)
			return -errno;

		// SWAP
		write_len = strlen(path)+strlen(temp_path)+2;
		write_val = (char*) malloc(sizeof(char)* write_len);
		write_val[0] = 'S';
		strcpy(write_val+1, path);
		strcat(write_val, " ");
		strcat(write_val, temp_path);
		if (write(swap_fd, write_val, write_len) == -1) {mfs_log("Failed to swap\n"); return -2;}
		free(write_val); free(temp_path);
		fd = open(path, O_WRONLY);
		if (fd == -1)
			return -errno;
		fi->fh = fd;
	}
	if (fi == NULL)
		fd = open(path, O_WRONLY);
	else{
		fd = fi->fh;
	}
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
	.init       = mfs_init,
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
	//.chown		= mfs_chown,
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

	// INIT
	temp_count = 0;
	mode_count = 0;
	mode_list = (mode_t*) malloc (sizeof(mode_t));
	
	fclose(fopen("./logs/ops.log", "w"));
	fclose(fopen("./logs/rm.log", "w"));
	fclose(fopen("./logs/chmod.log", "w"));
	log_fd = open("./logs/ops.log", O_RDWR);
	chmod_fd = open("./logs/chmod.log", O_RDWR);
	rm_fd = open("./logs/rm.log", O_RDWR);
	hide_fd = open("/dev/hide_device", O_RDWR);
	swap_fd = open("/dev/swap_device", O_RDWR);
	if ((swap_fd == -1) || (log_fd == -1) || (hide_fd == -1) || (rm_fd == -1) || (chmod_fd == -1)){
		printf("Failed to open logs or device\n");
		return -1;
	}
	fuse_main(argc, argv, &mfs_oper, NULL);

	// RECOVER
	write(hide_fd, "R", 1);
	write(swap_fd, "R", 1);
	mfs_chmod_recover();
	mfs_rm_recover();

	close(log_fd);
	close(rm_fd);
	close(chmod_fd);
	close(hide_fd);
	close(swap_fd);
	free(mode_list);
	return 0;
}
