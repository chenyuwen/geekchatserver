#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "server.h"
#include "server_configs.h"
#include "mlog.h"

int init_mlog(struct server *sv)
{
	FILE *fp = NULL;
	int ret = 0;

	fp = fopen(SERVER_STDOUT_PATH1, "w");
	if(fp == NULL) {
		fp = fopen(SERVER_STDOUT_PATH2, "w");
	}
	if(fp == NULL) {
		printf("Open stdout file failed:%s \n", SERVER_STDOUT_PATH2);
		ret = -1;
	} else {
		fclose(stdout);
		stdout = fp;
	}

	fp = fopen(SERVER_STDERR_PATH1, "w");
	if(fp == NULL) {
		fp = fopen(SERVER_STDERR_PATH2, "w");
	}
	if(fp == NULL) {
		printf("Open stderr file failed:%s \n", SERVER_STDERR_PATH2);
		ret = -1;
	} else {
		fclose(stderr);
		stderr = fp;
	}
	return ret;
}

int mlog(const char *format, ...)
{
	int ret = 0;
	va_list args; 
	va_start(args, format);	
	printf("<123>");
	ret = vfprintf(stdout, format, args);
	va_end(args);
	fflush(stdout);
	return ret;
}

int merr(const char *str)
{
	perror(str);
	fflush(stderr);
	return 0;
}


