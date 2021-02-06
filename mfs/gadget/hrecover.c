#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

int main(){
	int fd;
	if ((fd = open("/dev/hide_device", O_RDWR)) == -1){
		printf("HRECOVER: Failed to open Hide_device\n");
		return -errno;
	}
	if (write(fd, "R", 1) == -1){
		printf("HRECOVER: Failed to write\n");
		close(fd);
		return -errno;
	}
	close(fd);
	return 0;
}
