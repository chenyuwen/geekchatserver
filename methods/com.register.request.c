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
#include "../server_errno.h"
#include "../mlog.h"
#include "methods.h"

#define THIS_METHOD_RESPOND_NAME "com.register.respond"

int create_user_to_mysql(struct server *sv, const char *username, const char *password)
{
	struct mysql_config *config = sv->mysql_config;
	int ret = 0;
	char query[SERVER_MAX_PACKETS];
	struct user *tmp = NULL;

	snprintf(query, sizeof(query),"insert into users(username, password)"\
		" values ('%s', '%s')", username, password);
	if(sv->dump) mlog("%s\n", query);
	ret = mysql_real_query_affected(config, query);
	if(ret < 0) {
		mlog("mysql_query: %s\n", mysql_err_str(config));
		return ret;
	}

	if(ret != 1) {
		mlog("affected rows is not one.\n");
		return -1;
	}

	return ret;
}

int method_com_register_request(struct server *sv, struct client *ct, json_t *json)
{
	json_error_t json_err;
	json_t *rsp_json = json_object();
	const char *username = NULL, *password = NULL;
	struct raw_packet *packet = malloc_raw_packet(sv, ct);
	int ret = 0, res_ret = 0;

	username = json_string_value(json_object_get(json, "username"));
	if(username == NULL) {
		mlog("Warning: The message did not have username.\n");
		res_ret = -SERR_ARG;
		goto respond;
	}

	password = json_string_value(json_object_get(json, "password"));
	if(password == NULL) {
		mlog("Warning: The message did not have password.\n");
		res_ret = -SERR_ARG;
		goto respond;
	}

	if(create_user_to_mysql(sv, username, password) < 0) {
		res_ret = -SERR_ERR;
		goto respond;
	}

respond:
	build_simplify_json(rsp_json, THIS_METHOD_RESPOND_NAME, res_ret);
	json_to_raw_packet(rsp_json, PACKET_TYPE_UNENCRY, packet);
	respond_raw_packet(sv, ct, packet);

out:
	/*free json*/
	json_delete(rsp_json);
	free_raw_packet(sv, ct, packet);
	return 0;
}

