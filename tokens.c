#include "server.h"
#include "server_configs.h"
#include "hex.h"
#include "users.h"
#include "tokens.h"

int init_token(struct server *sv)
{
	sv->tokens_map = hashmap_new();
	return 0;
}

int alloc_new_token(struct server *sv, struct user *usr)
{
	unsigned char raw_token[SERVER_TOKEN_LENS / 2];
	int ret = get_random_bytes(&sv->random, raw_token, sizeof(raw_token));
	if(ret < 0) {
		return ret;
	}

	/*TODO: check the token*/
	hex_to_ascii(usr->token, raw_token, sizeof(raw_token));
	hashmap_put(sv->tokens_map, usr->token, usr);
	user_get(sv, usr);
	return ret;
}

int free_token(struct server *sv, struct user *usr)
{
	hashmap_remove(usr, usr->token);
	user_put(sv, usr);
	return 0;
}

int get_usr_by_token(struct server *sv, const char *token, struct user **usr)
{
	int ret = hashmap_get(sv->tokens_map, (char *)token, (any_t *)usr);
	user_get(sv, *usr);
	return ret;
}

