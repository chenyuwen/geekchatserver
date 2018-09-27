#include <string.h>
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

int store_token_to_mysql(struct server *sv, struct user *usr)
{
	struct mysql_config *config = sv->mysql_config;
	int ret = 0;
	char query[200];

	snprintf(query, sizeof(query),"update users set token = '%s', token_valid = '1'" \
		" where username = '%s'", usr->token, usr->username);
	if(sv->dump) mlog("%s\n", query);
	ret = mysql_real_query_affected(config, query);
	if(ret < 0) {
		mlog("mysql_query: %s\n", mysql_error(config->mysql));
		return ret;
	}

	if(ret != 1) {
		mlog("affected rows is not one.\n");
		return -1;
	}

	return 0;
}

int get_username_by_token_from_mysql(struct server *sv, const char *token,
	char *username, int size)
{
	struct mysql_config *config = sv->mysql_config;
	MYSQL_RES *result = NULL;
	MYSQL_ROW row = NULL;
	int ret = 0;
	char query[200];

	snprintf(query, sizeof(query),"select username, token from users" \
		" where token = '%s' and token_valid = '1'", token);
	if(sv->dump) mlog("%s\n", query);
	ret = mysql_real_query_result(config, query, &result);
	if(ret < 0 || result == NULL) {
		mlog("mysql_query: %s\n", mysql_error(config->mysql));
		return ret;
	}

	if(!mysql_num_rows(result)) {
		ret = -1;
		goto out;
	} else if(mysql_num_rows(result) >= 2) {
		mlog("Error: The username '%s' was the same.\n", username);
		ret = -1;
		goto out;
	}
	row = mysql_fetch_row(result);
	strncpy(username, row[0], SERVER_USERNAME_LENS);

out:
	mysql_free_result(result);
	return ret;
}

int alloc_new_token(struct server *sv, struct user *usr)
{
	unsigned char raw_token[SERVER_TOKEN_LENS / 2];
	int ret = get_random_bytes(&sv->random, raw_token, sizeof(raw_token));
	if(ret < 0) {
		return ret;
	}

	if(is_token_effective(sv, usr)) {
		free_token(sv, usr);
	}

	hex_to_ascii(usr->token, raw_token, sizeof(raw_token));
	ret = store_token_to_mysql(sv, usr);
	if(ret < 0) {
		return ret;
	}
	return try_effect_token(sv, usr);
}

int try_effect_token(struct server *sv, struct user *usr)
{
	if(!strcmp("NULL", usr->token)) {
		return -1;
	}

	hashmap_put(sv->tokens_map, usr->token, usr);
	user_get(sv, usr);
	return 0;
}

int free_token(struct server *sv, struct user *usr)
{
	hashmap_remove(sv->tokens_map, usr->token);
	mlog("free token:%s\n", usr->username);
	user_put(sv, usr);
	return 0;
}

int get_usr_by_token(struct server *sv, struct client *ct,
	const char *token, struct user **usr)
{
	int ret = hashmap_get(sv->tokens_map, (char *)token, (any_t *)usr);
	char username[SERVER_USERNAME_LENS + 1] = {0};
	if(ret == 0 && *usr != NULL) {
		user_get(sv, *usr);
		goto out;
	}

	ret = get_username_by_token_from_mysql(sv, token, username, SERVER_USERNAME_LENS);
	if(ret < 0) {
		*usr = NULL;
		goto out;
	}
	ret = get_user_by_name(sv, username, usr);
	if(ret < 0 || *usr == NULL) {
		goto out;
	}
	bind_user_to_client(sv, *usr, ct);

out:
	return ret;
}

int is_token_in_memory(struct server *sv, struct user *usr)
{
	struct user *out = NULL;
	int ret = 0;
	ret = hashmap_get(sv->tokens_map, (char *)usr->token, (any_t *)&out);
	if(ret < 0 || out == NULL) {
		mlog("token is not in memory\n");
		return 0;
	}
	return 1;
}

int is_token_effective(struct server *sv, struct user *usr)
{
	return is_token_in_memory(sv, usr);
}

