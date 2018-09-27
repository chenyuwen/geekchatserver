#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <mysql/mysql.h>
#include "packet.h"
#include "server.h"
#include "users.h"
#include "err.h"
#include "mlog.h"
#include "friends.h"
#include "timer.h"
#include "tokens.h"
#include "messages.h"
#include "server_configs.h"

int init_users_map(struct server *sv)
{
	int ret = 0;
	sv->users_map = hashmap_new();

	return 0;
}

int user_get(struct server *sv, struct user *usr)
{
	usr->use_cnt++;
	printf("user_get usr:%s cnt:%d\n", usr->username, usr->use_cnt);
	if(is_timer_effective(sv, &usr->timer)) {
		kick_timer(sv, &usr->timer);
	}
	return 0;
}

int user_put(struct server *sv, struct user *usr)
{
	usr->use_cnt--;
	printf("user_put usr:%s cnt:%d\n", usr->username, usr->use_cnt);
	if(usr->use_cnt == 0) {
		mlog("Free user:%s\n", usr->username);
		hashmap_remove(sv->users_map, usr->username);
		if(usr->friends.is_inited) {
			free_friends(usr);
		}
		if(is_user_bind(usr)) {
			unbind_user(usr);
		}
		if(is_timer_effective(sv, &usr->timer)) {
			printf("%s %d del_timer\n", __func__, __LINE__);
			//del_timer(sv, &usr->timer);
		}
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

	snprintf(query, sizeof(query),"select username, password, token from "
		"users where username = '%s'", username);
	if(sv->dump) mlog("%s\n", query);
	ret = mysql_real_query(config->mysql, query, strlen(query));
	if(ret < 0) {
		mlog("mysql_query: %s %d\n", mysql_error(config->mysql), -mysql_errno(config->mysql));
		return -mysql_errno(config->mysql);
	}

	result = mysql_store_result(config->mysql);
	if(result == NULL) {
		mlog("mysql_store_result: %s %d\n", mysql_error(config->mysql),  -mysql_errno(config->mysql));
		return -mysql_errno(config->mysql);
	}

	if(!mysql_num_rows(result)) {
		ret = -1;
		goto out;
	} else if(mysql_num_rows(result) >= 2) {
		mlog("Error: The username '%s' was the same.\n", username);
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
	strncpy(tmp->username, row[0], SERVER_USERNAME_LENS);
	strncpy(tmp->password, row[1], SERVER_PASSWORD_LENS);
	if(row[2] != NULL) {
		strncpy(tmp->token, row[2], SERVER_TOKEN_LENS);
	}
	if(sv->dump) mlog("usrname:%s, password:%s token:%s\n", tmp->username,
		tmp->password, tmp->token);
	ret = hashmap_put(sv->users_map, tmp->username, tmp);
	if(ret != MAP_OK) {
		mlog("Error: The user %s had been in the hashtable\n", username);
		free(tmp);
		ret = -1;
		goto out;
	}
	*usr = tmp;

out:
	mysql_free_result(result);
	return ret;
}

static int user_memory_free_handler(struct cbtimer *timer, void *arg)
{
	struct user *usr = container_of(timer, struct user, timer);
	struct server *sv = (struct server *)arg;

	if(is_token_in_memory(sv, usr)) {
		free_token(sv, usr);
	}
	user_put(sv, usr);
	return 0;
}

int get_user_by_name(struct server *sv, const char *username, struct user **usr)
{
	int ret = 0;

	ret = hashmap_get(sv->users_map, (char *)username, (any_t *)usr);
	if(usr == NULL || ret != MAP_OK) {
		ret = get_user_by_name_from_mysql(sv, username, usr);
		if(ret < 0) {
			usr = NULL;
			goto err;
		}

		/*memory auto free*/
		(*usr)->timer.handler = user_memory_free_handler;
		(*usr)->timer.arg = (void *)sv;
		add_timer(sv, &(*usr)->timer, SERVER_USER_UNUSED_TIMEOUT);
		user_get(sv, *usr); /*cache*/

		/*register token*/
		try_effect_token(sv, *usr);
	}
	user_get(sv, *usr);

err:
	return ret;
}

int bind_user_to_client(struct server *sv, struct user *usr, struct client *ct)
{
	ct->usr = usr;
	usr->client = ct;

	ct->looper.sv = sv;
	ct->looper.ct = ct;
	start_message_looper(&ct->looper);
	return 0;
}

int unbind_user(struct user *usr)
{
	if(is_message_looper_alive(&usr->client->looper)) {
		stop_message_looper(&usr->client->looper);
	}

	usr->client->usr = NULL;
	usr->client = NULL;
	return 0;
}

int is_user_bind(struct user *usr)
{
	if(usr == NULL) {
		return 0;
	} else if(usr->client == NULL) {
		return 0;
	}
	return 1;
}

int json_to_user(struct server *sv, json_t *json)
{
	return 0;
}

