#ifndef __FRIENDS_H__
#define __FRIENDS_H__

#include "c_hashmap/hashmap.h"

struct friends {
	int num_of_friends;
	unsigned char *buffer;
	map_t friends_map;
};

int list_my_friends(struct server *sv, struct user *usr);
int get_friends_by_user_from_mysql(struct server *sv, struct user *usr);

#endif /*__FRIENDS_H__*/

