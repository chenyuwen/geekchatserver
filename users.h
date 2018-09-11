#ifndef __USER_H__
#define __USER_H__

#include "server.h"

struct user {
	char appid[20];
	char username[20];
};

int init_users_map(struct server *sv);
int get_user_by_name(struct server *sv, const char *name, struct user **usr);

#endif /*__USER_H__*/

