#include "server.h"
#include "server_configs.h"
#include "hex.h"
#include "users.h"
#include "tokens.h"
#include "mlog.h"

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
	hashmap_remove(sv->tokens_map, usr->token);
	mlog("free token:%s\n", usr->username);
	user_put(sv, usr);
	return 0;
}

int get_usr_by_token(struct server *sv, const char *token, struct user **usr)
{
	int ret = hashmap_get(sv->tokens_map, (char *)token, (any_t *)usr);
	if(ret == 0 && usr != NULL) {
		user_get(sv, *usr);
	}
	return ret;
}

int is_token_in_memory(struct server *sv, struct user *usr)
{
	struct user *out = NULL;
	int ret = 0;
	ret = hashmap_get(sv->tokens_map, (char *)usr->token, (any_t *)&out);
	if(ret < 0 || out == NULL) {
		printf("token is not in memory\n");
		return 0;
	}
	return 1;
}

int is_token_effective(struct server *sv, struct user *usr)
{
	return is_token_in_memory(sv, usr);
}

