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
#include "methods.h"

int method_com_login_seed_request(struct server *sv, struct client *ct, json_t *json)
{
	json_error_t json_err;
	json_t *rsp_json = json_object();
	struct raw_packet *packet = malloc_raw_packet(sv, ct);
	struct user *usr = NULL;
	const char *username = NULL;
	int ret = 0;

	printf("%s\n", __func__);
	username = json_string_value(json_object_get(json, "username"));
	if(username == NULL) {
		/*TODO: Can't find this user.*/
		build_not_found_json(sv, ct, rsp_json, "com.login.seed.respond");
		goto respond;
	}
	printf("username:%s\n", username);
	ret = get_user_by_name(sv, username, &usr);
	if(ret == -ERR_WAIT) {
		/*TODO: Add to waitqueue*/
		printf("Not found\n");
		build_not_found_json(sv, ct, rsp_json, "com.login.seed.respond");
		goto respond;
	}

	json_object_set_new(rsp_json, "method", json_string("com.login.seed.respond"));
	json_object_set_new(rsp_json, "seed", json_string("random"));
	json_object_set_new(rsp_json, "username", json_string(usr->username));
	json_object_set_new(rsp_json, "status", json_true());

respond:
	json_to_raw_packet(rsp_json, PACKET_TYPE_UNENCRY, packet);
	respond_raw_packet(sv, ct, packet);

out:
	/*free json*/
	json_delete(rsp_json);
	free_raw_packet(sv, ct, packet);
	return 0;
}

