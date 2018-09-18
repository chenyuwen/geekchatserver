#ifndef __USER_H__
#define __USER_H__

#include <stdint.h>
#include "server.h"
#include "server_configs.h"

struct user {
	uint32_t usrid;
	char token[SERVER_TOKEN_LENS + 1];
	char seed[SERVER_SEED_LENS + 1];
	char username[SERVER_USERNAME_LENS + 1];
	char password[SERVER_PASSWORD_LENS + 1];
	int is_online;
	int use_cnt;
	int is_seed_existed;
	struct client *client;
};

int init_users_map(struct server *sv);
int get_user_by_name(struct server *sv, struct client *ct, const char *name,
	struct user **usr);
int get_user_by_name_using_cache(struct server *sv, struct client *ct, const char *name,
	struct user **usr);
int user_get(struct server *sv, struct user *usr);
int user_put(struct server *sv, struct user *usr);
int bind_user(struct server *sv, struct client *ct, struct user *usr);
int unbind_user(struct server *sv, struct client *ct, struct user *usr);
int free_user(struct user *usr);

#endif /*__USER_H__*/

