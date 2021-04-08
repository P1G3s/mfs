extern int mv_fd;
extern int rm_fd;
extern int log_fd;
extern int chmod_fd;
extern mode_t* mode_list;

static void mfs_log(const char* msg){
	write(log_fd, msg, strlen(msg));
}

static void mfs_rm_log(const char* msg){
	write(rm_fd, "\n", 1);
	write(rm_fd, msg, strlen(msg));
}

static void mfs_chmod_log(const char* msg){
	write(chmod_fd, "\n", 1);
	write(chmod_fd, msg, strlen(msg));
}

static void mfs_mv_log(const char* msg){
	write(mv_fd, "\n", 1);
	write(mv_fd, msg, strlen(msg));
}


static int copy_to_temp(const char* src_path, const char* temp_path){
	int src_fd, temp_fd;
	int len;
	int buf_size = 1024;
	char buf[buf_size];
	
	temp_fd = open(temp_path, O_RDWR | O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
	if (temp_fd == -1) {perror("copy_to_temp(open)"); return -errno;}
	src_fd = open(src_path, O_RDONLY);
	lseek(src_fd, 0, SEEK_SET);
	while ((len=read(src_fd, buf, buf_size)) > 0){
		if (write(temp_fd, buf, len) != len) {perror("copy_to_temp(write)"); return -errno;}
	}
	close(src_fd);
   	close(temp_fd);
	return 0;
}

static void mfs_rm_recover(){
	int mark_arr[1000];	
	int i = 0;
	int j = 0;
	int len;
	char mark;
	char buf[MAXLEN];

	printf("RECOVER(rm)...\n");
	lseek(rm_fd, 0, SEEK_SET);
	while (read(rm_fd, &mark, 1) == 1){
		if (mark == '\n'){
			mark_arr[i] = j;
			i++;
		}
		j++;
	}
	i-=1;
	if (i <= 0) return;

	for (; i>0; i--){
		len = mark_arr[i]-mark_arr[i-1]-1;
		lseek(rm_fd, mark_arr[i-1]+1, SEEK_SET);
		read(rm_fd, buf, len);
		buf[len] = '\0';
		if (unlink(buf) == -1 && errno == EISDIR)
			rmdir(buf);
	}
}

static void mfs_chmod_recover(){
	int mark_arr[1000];	// LAZY ALLOCATION :(
	int i = 0;
	int j = 0;
	int len;
	char mark;
	char buf[MAXLEN];

	printf("RECOVER(chmod)...\n");
	lseek(chmod_fd, 0, SEEK_SET);
	while (read(chmod_fd, &mark, 1) == 1){
		if (mark == '\n'){
			mark_arr[i] = j;
			i++;
		}
		j++;
	}
	i-=1;
	if (i <= 0) return;

	for (; i>0; i--){
		len = mark_arr[i]-mark_arr[i-1]-1;
		lseek(chmod_fd, mark_arr[i-1]+1, SEEK_SET);
		read(chmod_fd, buf, len);
		buf[len] = '\0';
		chmod(buf, mode_list[i]);
	}
}

static void mfs_mv_recover(){
	int mark_arr[1000];	// LAZY ALLOCATION :(
	int i = 0;
	int j = 0;
	int len1;
	int len2;
	char mark;
	char buf1[MAXLEN];
	char buf2[MAXLEN];

	printf("RECOVER(mv)...\n");
	lseek(mv_fd, 0, SEEK_SET);
	while (read(mv_fd, &mark, 1) == 1){
		if (mark == '\n'){
			mark_arr[i] = j;
			i++;
		}
		j++;
	}
	i-=1;
	if (i <= 0) return;
	
	for (; i>1; i-=2){
		// CURRENT NAME
		len1 = mark_arr[i]-mark_arr[i-1]-1;
		lseek(mv_fd, mark_arr[i-1]+1, SEEK_SET);
		read(mv_fd, buf1, len1);
		buf1[len1] = '\0';
		// TARGET NAME
		len2 = mark_arr[i-1]-mark_arr[i-2]-1;
		lseek(mv_fd, mark_arr[i-2]+1, SEEK_SET);
		read(mv_fd, buf2, len2);
		buf2[len2] = '\0';

		printf("rename: %s -> %s\n", buf1, buf2);
		rename(buf1, buf2);
	}
}
