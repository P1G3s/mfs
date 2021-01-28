#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "device.h"

int swap_inode(const char* src_name, const char* des_name){
	int fd,len;
	char* write_val;

	// PREP THE STRING
	len = strlen(src_name) + strlen(des_name) + 1;
	write_val = (char*) malloc (sizeof(char)*len);
	strcat(write_val, src_name);
	strcat(write_val, " ");
	strcat(write_val, des_name);

	// WRITE THE DEVICE WITH STRING
	fd = open(DEVICE_PATH, O_RDWR); 
	if(fd == -1) perror("SWAP_FILE_ERR");
	if(write(fd, write_val, len) == -1) perror("SWAP_FILE_ERR");
	close(fd);
}

int is_swapped(const char* file_name){

}
