#ifndef __FRIENDS_H__
#define __FRIENDS_H__

#include "c_hashmap/hashmap.h"

struct friends {
	int is_inited;
	int num_of_friends;
	unsigned char *buffer;
	map_t friends_map;
};

int get_friends_by_user(struct server *sv, struct user *usr, struct friends **friends);
int get_friends_by_user_from_mysql(struct server *sv, struct user *usr);
int free_friends(struct user *usr);
int friends_put(struct friends *friends);
int make_friend(struct server *sv, struct user *usr, struct user *friend);

#endif /*__FRIENDS_H__*/

