#ifndef __USER_H__
#define __USER_H__

#include "server.h"

struct user {
	char appid[20];
	char username[20];
	int is_online;
	int use_cnt;
};

int init_users_map(struct server *sv);
int get_user_by_name(struct server *sv, struct client *ct, const char *name,
	struct user **usr);
int free_user(struct user *usr);
int user_get(struct server *sv, struct user *usr);
int user_put(struct server *sv, struct user *usr);

#endif /*__USER_H__*/

