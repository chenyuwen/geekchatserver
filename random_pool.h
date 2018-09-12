#ifndef __RANDOM_POOL_H__
#define __RANDOM_POOL_H__

struct random_pool {
	int random_fd;
};

int init_random_pool(struct random_pool *pool);
int get_random_bytes(struct random_pool *pool, char *byte, int len);
int exit_random_pool(struct random_pool *pool);

#endif /*__RANDOM_POOL_H__*/

