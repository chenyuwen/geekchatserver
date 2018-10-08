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
#include "../users.h"
#include "../tokens.h"
#include "../mlog.h"
#include "../server_errno.h"
#include "../friends.h"
#include "methods.h"

#define THIS_METHOD_RESPOND_NAME "com.friends.add.respond"

int method_com_friends_add_request(struct server *sv, struct client *ct, json_t *json)
{
	json_error_t json_err;
	json_t *rsp_json = json_object();
	struct raw_packet *packet = alloc_raw_packet(sv, ct);
	struct user *usr = NULL, *friend = NULL;
	const char *token, *friend_name;
	int ret = 0, res_ret = 0;

	token = json_string_value(json_object_get(json, "token"));
	if(token == NULL) {
		mlog("Warning: The message did not have token.\n");
		res_ret = -SERR_ARG;
		goto respond;
	}

	ret = get_usr_by_token(sv, ct, token, &usr);
	if(ret < 0 || usr == NULL) {
		mlog("Warning: The token was invaild.\n");
		res_ret = -SERR_FORCE_LOGOUT;
		goto respond;
	}

	friend_name = json_string_value(json_object_get(json, "friend"));
	if(friend_name == NULL) {
		mlog("Warning: The message did not have token.\n");
		res_ret = -SERR_ARG;
		user_put(sv, usr);
		goto respond;
	}

	ret = get_user_by_name(sv, friend_name, &friend);
	if(ret < 0 || friend == NULL) {
		mlog("Warning: The token was invaild.\n");
		res_ret = -SERR_USR_NOT_EXIST;
		user_put(sv, usr);
		goto respond;
	}

	if(make_friend(sv, usr, friend) < 0) {
		res_ret = -SERR_ERR;
		user_put(sv, usr);
		user_put(sv, friend);
		goto respond;
	}
	user_put(sv, usr);
	user_put(sv, friend);

respond:
	build_simplify_json(rsp_json, THIS_METHOD_RESPOND_NAME, res_ret);
	json_to_raw_packet(rsp_json, PACKET_TYPE_UNENCRY, packet);
	respond_raw_packet(sv, ct, packet);

	/*free json*/
	json_delete(rsp_json);
	free_raw_packet(sv, ct, packet);
	return 0;
}

