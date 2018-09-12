#include "server.h"
#include "stdio.h"

int init_random_pool(struct server *sv)
{
	return 0;
}

int get_random_bytes(struct server *sv, char *byte, int len)
{
	return read(sv->random_fds_map, byte, len);
}

