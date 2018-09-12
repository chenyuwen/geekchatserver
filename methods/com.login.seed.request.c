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
#include "methods.h"

int build_new_token(struct server *sv, struct user *usr)
{
	unsigned char raw_token[SERVER_TOKEN_LENS / 2];
	int ret = get_random_bytes(&sv->random, raw_token, sizeof(raw_token));
	if(ret < 0) {
		return ret;
	}

	return hex_to_ascii(usr->token, raw_token, sizeof(raw_token));
}

int build_new_seed(struct server *sv, struct user *usr)
{
	unsigned char raw_seed[SERVER_TOKEN_LENS / 2];
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
	struct raw_packet *packet = malloc_raw_packet(sv, ct);
	struct user *usr = NULL;
	const char *username = NULL;
	char *token;
	int ret = 0;

	username = json_string_value(json_object_get(json, "username"));
	if(username == NULL) {
		printf("Warning: The packet did not have username.\n");
		build_not_found_json(sv, ct, rsp_json, "com.login.seed.respond");
		goto respond;
	}
	ret = get_user_by_name(sv, ct, username, &usr);
	if(ret < 0) {
		/*Can't find this user.*/
		printf("Warning: The user '%s' did not registered.\n", username);
		build_not_found_json(sv, ct, rsp_json, "com.login.seed.respond");
		goto respond;
	}

	if(build_new_seed(sv, usr) < 0) {
		build_not_found_json(sv, ct, rsp_json, "com.login.seed.respond");
		goto respond;
	}
	json_object_set_new(rsp_json, "method", json_string("com.login.seed.respond"));
	json_object_set_new(rsp_json, "seed", json_string(usr->seed));
	json_object_set_new(rsp_json, "username", json_string(usr->username));
	json_object_set_new(rsp_json, "status", json_true());
	user_put(sv, usr);

respond:
	json_to_raw_packet(rsp_json, PACKET_TYPE_UNENCRY, packet);
	respond_raw_packet(sv, ct, packet);

out:
	/*free json*/
	json_delete(rsp_json);
	free_raw_packet(sv, ct, packet);
	return 0;
}

