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
#include "../err.h"
#include "../hex.h"
#include "../mlog.h"
#include "../server_errno.h"
#include "methods.h"

#define THIS_METHOD_RESPOND_NAME "com.login.seed.respond"

int build_new_seed(struct server *sv, struct user *usr)
{
	unsigned char raw_seed[SERVER_SEED_LENS / 2];
	int ret = get_random_bytes(&sv->random, raw_seed, sizeof(raw_seed));
	if(ret < 0) {
		return ret;
	}

	return hex_to_ascii(usr->seed, raw_seed, sizeof(raw_seed));
}

int method_com_login_seed_request(struct server *sv, struct client *ct, json_t *json)
{
	json_error_t json_err;
	json_t *rsp_json = json_object();
	struct raw_packet *packet = alloc_raw_packet(sv, ct);
	struct user *usr = NULL;
	const char *username = NULL;
	char *token;
	int ret = 0, res_ret = SERR_SUCCESS;

	username = json_string_value(json_object_get(json, "username"));
	if(username == NULL) {
		mlog("Warning: The packet did not have username.\n");
		res_ret = -SERR_ARG;
		goto respond;
	}
	ret = get_user_by_name(sv, username, &usr);
	if(ret < 0) {
		/*Can't find this user.*/
		mlog("Warning: The user '%s' did not registered.\n", username);
		res_ret = -SERR_USR_NOT_EXIST; /*TODO:*/
		goto respond;
	}

	if(build_new_seed(sv, usr) < 0) {
		user_put(sv, usr);
		res_ret = -SERR_ARG;
		goto respond;
	}
	usr->is_seed_existed = 1;
	json_object_set_new(rsp_json, "seed", json_string(usr->seed));
	json_object_set_new(rsp_json, "username", json_string(usr->username));
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

