#include <string.h>
#include "server.h"
#include "users.h"
#include "server_configs.h"
#include "friends.h"
#include "mlog.h"

int get_friends_by_user_from_mysql(struct server *sv, struct user *usr)
{
	struct mysql_config *config = sv->mysql_config;
	MYSQL_RES *result = NULL;
	MYSQL_ROW row = NULL;
	int ret = 0, num_of_rows = 0, offset = 0;
	char query[100], *tmp = NULL;

	snprintf(query, sizeof(query),"select username, friend from "
		"friends where username = '%s'", usr->username);
	ret = mysql_query(config->mysql, query);
	if(ret < 0) {
		mlog("mysql_query: %s\n", mysql_error(config->mysql));
		return -mysql_errno(config->mysql);
	}

	result = mysql_store_result(config->mysql);
	if(result == NULL) {
		mlog("mysql_store_result: %s\n", mysql_error(config->mysql));
		return -mysql_errno(config->mysql);
	}

	num_of_rows = mysql_num_rows(result);
	usr->friends.buffer = malloc(num_of_rows * (SERVER_USERNAME_LENS + 1));
	if(usr->friends.buffer == NULL) {
		perror("malloc");
		ret = -1;
		goto out;
	}
	memset(usr->friends.buffer, 0, num_of_rows * SERVER_USERNAME_LENS); 
	usr->friends.friends_map = hashmap_new();
	if(usr->friends.friends_map == NULL) {
		printf("malloc friends_map failed.\n");
		ret = -1;
		goto out;
	}

	while(row = mysql_fetch_row(result)) {
		tmp = usr->friends.buffer + (SERVER_USERNAME_LENS + 1) * offset;
		strncpy(tmp, row[1], SERVER_USERNAME_LENS);
		if(sv->dump) mlog("username:%s, friend:%s\n", row[0], row[1]);

		ret = hashmap_put(usr->friends.friends_map, tmp, "1");
		if(ret != MAP_OK) {
			mlog("Error: The user %s friend %s is misake.\n", row[0], row[1]);
			free(usr->friends.buffer);
			usr->friends.buffer = NULL;
			ret = -1;
			goto out;
		}
		offset++;
	}
	usr->friends.num_of_friends = num_of_rows;

out:
	mysql_free_result(result);
	return ret;
}

