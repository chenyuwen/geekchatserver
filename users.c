#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "packet.h"
#include "server.h"
#include "users.h"
#include "err.h"

int init_users_map(struct server *sv)
{
	static struct user usr1 = {"12345", "zhangsan"};
	static struct user usr2 = {"23456", "lisi"};
	int ret = 0;
	sv->users_map = hashmap_new();

	ret = hashmap_put(sv->users_map, usr1.username, &usr1);
	ret = hashmap_put(sv->users_map, usr2.username, &usr2);
	return 0;
}

int get_user_by_name(struct server *sv, const char *username, struct user **usr)
{
	int ret = hashmap_get(sv->users_map, (char *)username, (any_t *)usr);
	if(*usr == NULL || ret != MAP_OK) {
		/*TODO: from database*/
		return -ERR_WAIT;
	}
	return 0;
}

int json_to_user(struct server *sv, json_t *json)
{
	return 0;
}

