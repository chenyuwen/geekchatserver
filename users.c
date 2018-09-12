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
	int ret = 0;
	sv->users_map = hashmap_new();

	return 0;
}

int user_get(struct server *sv, struct user *usr)
{
	usr->use_cnt++;
	return 0;
}

int user_put(struct server *sv, struct user *usr)
{
	usr->use_cnt--;
	if(usr->use_cnt == 0) {
		printf("Free user:%s\n", usr->username);
		hashmap_remove(sv->users_map, usr->username);
		free(usr);
	}
	return 0;
}

int get_user_by_name_from_mysql(struct server *sv, const char *username, struct user **usr)
{
	struct mysql_config *config = sv->mysql_config;
	MYSQL_RES *result = NULL;
	MYSQL_ROW row = NULL;
	int ret = 0;
	char query[100];
	struct user *tmp = NULL;

	snprintf(query, sizeof(query),"select * from users where username = '%s'", username);
	ret = mysql_query(config->mysql, query);
	if(ret < 0) {
		printf("mysql_query: %s\n", mysql_error(config->mysql));
		return -mysql_errno(config->mysql);
	}

	result = mysql_store_result(config->mysql);
	if(result == NULL) {
		printf("mysql_store_result: %s\n", mysql_error(config->mysql));
		return -mysql_errno(config->mysql);
	}

	if(!mysql_num_rows(result)) {
		ret = -1;
		goto out;
	} else if(mysql_num_rows(result) >= 2) {
		printf("Error: The username '%s' was the same.\n", username);
		ret = -1;
		goto out;
	}

	tmp = (void *)malloc(sizeof(struct user));
	if(tmp == NULL) {
		ret = -1;
		goto out;
	}

	memset(tmp, 0, sizeof(*tmp));
	row = mysql_fetch_row(result);
	strncpy(tmp->username, username, sizeof(tmp->username));
	ret = hashmap_put(sv->users_map, tmp->username, tmp);
	if(ret != MAP_OK) {
		printf("Error: The user %s had been in the hashtable\n", username);
		free(tmp);
		ret = -1;
		goto out;
	}
	*usr = tmp;

out:
	mysql_free_result(result);
	return ret;
}

int get_user_by_name(struct server *sv, struct client *ct, const char *username,
	struct user **usr)
{
	int ret = 0;
	if(ct->usr != NULL) {
		ret = strncmp(username, ct->usr->username, sizeof(ct->usr->username));
		if(ret == 0) {
			goto out;
		}
		user_put(sv, ct->usr);
		ct->usr = NULL;
	}

	ret = hashmap_get(sv->users_map, (char *)username, (any_t *)&ct->usr);
	if(ct->usr == NULL || ret != MAP_OK) {
		ret = get_user_by_name_from_mysql(sv, username, &ct->usr);
		if(ret < 0) {
			ct->usr = NULL;
			goto err;
		}
		user_get(sv, ct->usr);
	}

out:
	user_get(sv, ct->usr);
err:
	*usr = ct->usr;
	return ret;
}

int json_to_user(struct server *sv, json_t *json)
{
	return 0;
}

