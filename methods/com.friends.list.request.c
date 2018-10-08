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
#include "../friends.h"
#include "../server_errno.h"
#include "methods.h"

#define THIS_METHOD_RESPOND_NAME "com.friends.list.respond"

int method_com_friends_list_request(struct server *sv, struct client *ct, json_t *json)
{
	json_error_t json_err;
	json_t *rsp_json = json_object(), *array = json_array();
	struct raw_packet *packet = alloc_raw_packet(sv, ct);
	const char *token, *tmp;
	struct user *usr;
	struct friends *friends = NULL;
	int ret = 0, i = 0, res_ret = SERR_SUCCESS;

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

	ret = get_friends_by_user(sv, usr, &friends);
	if(ret < 0 || friends == NULL) {
		mlog("Warning: The token was invaild.\n");
		res_ret = -SERR_FORCE_LOGOUT;
		user_put(sv, usr);
		goto respond;
	}

	for(i=0; i<usr->friends.num_of_friends; i++) {
		tmp = usr->friends.buffer + (SERVER_USERNAME_LENS + 1) * i;
		printf("friends:%s\n", tmp);
		json_array_append(array, json_string(tmp));
	}
	json_object_set_new(rsp_json, "friends", array);
	friends_put(friends);
	user_put(sv, usr);

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

