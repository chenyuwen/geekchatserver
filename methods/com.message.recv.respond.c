#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../c_hashmap/hashmap.h"
#include "../crc32/crc32.h"
#include "../jansson/jansson.h"
#include "../server_configs.h"
#include "../packet.h"
#include "../server.h"
#include "../tokens.h"
#include "../users.h"
#include "../mlog.h"
#include "methods.h"

int set_message_read_to_mysql(struct server *sv, struct user *usr, const char *uuid)
{
	struct mysql_config *config = sv->mysql_config;
	int ret = 0;
	char query[SERVER_MAX_PACKETS];
	struct user *tmp = NULL;

	snprintf(query, sizeof(query),"update messages set have_read = '1'" \
		" where to_user = '%s' and uuid = '%s'", usr->username, uuid);
	if(sv->dump) mlog("%s\n", query);
	ret = mysql_real_query(config->mysql, query, strlen(query));
	if(ret < 0) {
		mlog("mysql_query: %s\n", mysql_error(config->mysql));
		return -mysql_errno(config->mysql);
	}

	if(mysql_affected_rows(config->mysql) != 1) {
		mlog("affected rows is not one.\n");
		return -1;
	}

	return ret;
}

int method_com_message_recv_respond(struct server *sv, struct client *ct, json_t *json)
{
	int ret = 0;
	const char *token = NULL, *uuid = NULL;
	struct user *usr;

	token = json_string_value(json_object_get(json, "token"));
	if(token == NULL) {
		mlog("Warning: The message did not have token.\n");
		goto out;
	}

	ret = get_usr_by_token(sv, ct, token, &usr);
	if(ret < 0 || usr == NULL) {
		mlog("Warning: The token was invaild.\n");
		goto out;
	}

	if(!is_token_effective(sv, usr)) {
		mlog("Warning: The user %s token is not effective\n", usr->username);
		user_put(sv, usr);
		goto out;
	}

	uuid = json_string_value(json_object_get(json, "uuid"));
	if(token == NULL) {
		mlog("Warning: The message did not have token.\n");
		goto out;
	}

	if(set_message_read_to_mysql(sv, usr, uuid) < 0) {
		user_put(sv, usr);
		goto out;
	}
	user_put(sv, usr);

out:
	return 0;
}

