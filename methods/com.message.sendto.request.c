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
#include "methods.h"

int send_message_to(struct server *sv, struct user *from, struct user *to,
	const char *message)
{
	json_t *rsp_json = json_object();
	struct raw_packet *packet = malloc_raw_packet(sv, to->client);

	json_object_set_new(rsp_json, "method", json_string("com.message.recv.request"));
	json_object_set_new(rsp_json, "status", json_true());
	json_object_set_new(rsp_json, "from", json_string(from->username));
	json_object_set_new(rsp_json, "token", json_string(to->token));
	json_object_set_new(rsp_json, "message", json_string(message));

respond:
	json_to_raw_packet(rsp_json, PACKET_TYPE_UNENCRY, packet);
	respond_raw_packet(sv, to->client, packet);

out:
	/*free json*/
	json_delete(rsp_json);
	free_raw_packet(sv, to->client, packet);
}

int method_com_message_sendto_request(struct server *sv, struct client *ct, json_t *json)
{
	json_error_t json_err;
	json_t *rsp_json = json_object();
	struct raw_packet *packet = malloc_raw_packet(sv, ct);
	const char *token = NULL, *sendto = NULL, *message = NULL;
	struct user *from, *to;
	int ret = 0;

	json_object_set_new(rsp_json, "method", json_string("com.message.sendto.respond"));
	json_object_set_new(rsp_json, "status", json_true());

	token = json_string_value(json_object_get(json, "token"));
	if(token == NULL) {
		mlog("Warning: The message did not have token.\n");
		build_not_found_json(sv, ct, rsp_json, "com.message.sendto.respond");
		goto respond;
	}

	ret = get_usr_by_token(sv, ct, token, &from);
	if(ret < 0 || from == NULL) {
		mlog("Warning: The token was invaild.\n");
		build_not_found_json(sv, ct, rsp_json, "com.message.sendto.respond");
		goto respond;
	}

	if(!is_token_effective(sv, from)) {
		/*TODO: write the message to databases*/
		mlog("Warning: The user %s token is not effective\n", from->username);
		build_not_found_json(sv, ct, rsp_json, "com.message.sendto.respond");
		user_put(sv, from);
		goto respond;
	}

	sendto = json_string_value(json_object_get(json, "sendto"));
	if(sendto == NULL) {
		mlog("Warning: The packet did not have username.\n");
		build_not_found_json(sv, ct, rsp_json, "com.message.sendto.respond");
		user_put(sv, from);
		goto respond;
	}

	ret = get_user_by_name(sv, ct, sendto, &to);
	if(to == NULL || ret < 0) {
		mlog("Warning: The user %s did not registerd\n", sendto);
		build_not_found_json(sv, ct, rsp_json, "com.message.sendto.respond");
		user_put(sv, from);
		goto respond;
	}

	if(to->client == NULL) {
		/*TODO: write the message to databases*/
		mlog("Warning: The user %s did not login\n", sendto);
		build_not_found_json(sv, ct, rsp_json, "com.message.sendto.respond");
		user_put(sv, from);
		user_put(sv, to);
		goto respond;
	}

	message = json_string_value(json_object_get(json, "message"));
	if(sendto == NULL) {
		mlog("Warning: The packet did not have username.\n");
		build_not_found_json(sv, ct, rsp_json, "com.message.sendto.respond");
		user_put(sv, from);
		user_put(sv, to);
		goto respond;
	}

	if(send_message_to(sv, from, to, message) < 0) {
		mlog("Warning: The message send to %s failed.\n", to->username);
		build_not_found_json(sv, ct, rsp_json, "com.message.sendto.respond");
		user_put(sv, from);
		user_put(sv, to);
		goto respond;
	}
	user_put(sv, from);
	user_put(sv, to);

respond:
	json_to_raw_packet(rsp_json, PACKET_TYPE_UNENCRY, packet);
	respond_raw_packet(sv, ct, packet);

out:
	/*free json*/
	json_delete(rsp_json);
	free_raw_packet(sv, ct, packet);
	return 0;
}

