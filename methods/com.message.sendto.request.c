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
#include "../mlog.h"
#include "../users.h"
#include "../tokens.h"
#include "../hex.h"
#include "../messages.h"
#include "../server_errno.h"
#include "methods.h"

#define THIS_METHOD_RESPOND_NAME "com.message.sendto.respond"

int write_message_to_mysql(struct server *sv, struct user *from, struct user *to,
	const char *uuid, const char *message)
{
	struct mysql_config *config = sv->mysql_config;
	int ret = 0;
	char query[SERVER_MAX_PACKETS];
	struct user *tmp = NULL;

	snprintf(query, sizeof(query),"insert into messages(uuid, from_user, to_user, message)"\
		" values ('%s', '%s', '%s', '%s')", uuid, from->username, to->username, message);
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

int allow_new_uuid(struct server *sv, unsigned char *uuid)
{
	unsigned char raw_uuid[SERVER_UUID_LENS / 2];

	memset(uuid, 0, SERVER_UUID_LENS + 1);
	get_random_bytes(&sv->random, raw_uuid, sizeof(raw_uuid));
	hex_to_ascii(uuid, raw_uuid, sizeof(raw_uuid));
	/*TODO: check the uuid*/
	return 0;
}

int method_com_message_sendto_request(struct server *sv, struct client *ct, json_t *json)
{
	json_error_t json_err;
	json_t *rsp_json = json_object();
	struct raw_packet *packet = alloc_raw_packet(sv, ct);
	const char *token = NULL, *sendto = NULL, *message = NULL;
	struct user *from, *to;
	int ret = 0, res_ret = SERR_SUCCESS;
	unsigned char uuid[SERVER_UUID_LENS + 1] = {0};

	token = json_string_value(json_object_get(json, "token"));
	if(token == NULL) {
		mlog("Warning: The message did not have token.\n");
		res_ret = -SERR_ARG;
		goto respond;
	}

	ret = get_usr_by_token(sv, ct, token, &from);
	if(ret < 0 || from == NULL) {
		mlog("Warning: The token was invaild.\n");
		res_ret = -SERR_FORCE_LOGOUT;
		goto respond;
	}

	if(!is_token_effective(sv, from)) {
		mlog("Warning: The user %s token is not effective\n", from->username);
		res_ret = -SERR_FORCE_LOGOUT;
		user_put(sv, from);
		goto respond;
	}

	sendto = json_string_value(json_object_get(json, "sendto"));
	if(sendto == NULL) {
		mlog("Warning: The packet did not have username.\n");
		res_ret = -SERR_ARG;
		user_put(sv, from);
		goto respond;
	}

	ret = get_user_by_name(sv, sendto, &to);
	if(to == NULL || ret < 0) {
		mlog("Warning: The user %s did not registerd\n", sendto);
		res_ret = -SERR_USR_NOT_EXIST;
		user_put(sv, from);
		goto respond;
	}

	message = json_string_value(json_object_get(json, "message"));
	if(sendto == NULL) {
		mlog("Warning: The packet did not have username.\n");
		res_ret = -SERR_ARG;
		user_put(sv, from);
		user_put(sv, to);
		goto respond;
	}

	allow_new_uuid(sv, uuid);
	if(to->client == NULL) {
		/*TODO: write the message to databases*/
		write_message_to_mysql(sv, from, to, uuid, message);
		mlog("Warning: Put the message to databases.\n");
		user_put(sv, from);
		user_put(sv, to);
		goto respond;
	}

	if(send_message_to(sv, from, to, uuid, message) < 0) {
		mlog("Warning: The message send to %s failed.\n", to->username);
		res_ret = -SERR_ERR;
		user_put(sv, from);
		user_put(sv, to);
		goto respond;
	}
	write_message_to_mysql(sv, from, to, uuid, message);
	user_put(sv, from);
	user_put(sv, to);

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

