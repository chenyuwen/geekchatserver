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
#include "../server_errno.h"
#include "../mlog.h"
#include "../tokens.h"
#include "methods.h"

#define THIS_METHOD_RESPOND_NAME "com.logout.respond"

int method_com_logout_request(struct server *sv, struct client *ct, json_t *json)
{
	json_error_t json_err;
	json_t *rsp_json = json_object();
	struct raw_packet *packet = malloc_raw_packet(sv, ct);
	const char *token = NULL;
	struct user *usr = NULL;
	int ret = 0, res_ret = SERR_SUCCESS;

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

	if(!is_token_effective(sv, usr)) {
		/*TODO: write the message to databases*/
		mlog("Warning: The user %s token is not effective\n", usr->username);
		res_ret = -SERR_FORCE_LOGOUT;
		user_put(sv, usr);
		goto respond;
	}

respond:
	build_simplify_json(rsp_json, THIS_METHOD_RESPOND_NAME, res_ret);
	json_to_raw_packet(rsp_json, PACKET_TYPE_UNENCRY, packet);
	respond_raw_packet(sv, ct, packet);

	/*free json*/
	json_delete(rsp_json);
	free_raw_packet(sv, ct, packet);
	return 0;
}

