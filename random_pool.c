#include <stdio.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "random_pool.h"
#include "server_configs.h"

int init_random_pool(struct random_pool *pool)
{
	pool->random_fd = open(URANDOM_FILE_DIR, O_RDONLY);
	if(pool->random_fd < 0) {
		perror("open");
		return errno;
	}
	return 0;
}

int get_random_bytes(struct random_pool *pool, char *byte, int len)
{
	return read(pool->random_fd, byte, len);
}

int exit_random_pool(struct random_pool *pool)
{
	close(pool->random_fd);
	return 0;
}

