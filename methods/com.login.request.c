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
#include "../crypto/sha256.h"
#include "../jansson/jansson.h"
#include "../server_configs.h"
#include "../packet.h"
#include "../server.h"
#include "../users.h"
#include "../hex.h"
#include "../mlog.h"
#include "../tokens.h"
#include "methods.h"

int method_com_login_request(struct server *sv, struct client *ct, json_t *json)
{
	json_error_t json_err;
	json_t *rsp_json = json_object();
	struct raw_packet *packet = malloc_raw_packet(sv, ct);
	const char *username = NULL, *crypto = NULL;
	unsigned char crypto_out[SHA256_LENS];
	unsigned char hex_out[SHA256_LENS * 2];
	SHA256_CTX ctx;
	struct user *usr;
	int ret = 0;

	username = json_string_value(json_object_get(json, "username"));
	if(username == NULL) {
		mlog("Warning: The packet did not have username.\n");
		build_not_found_json(sv, ct, rsp_json, "com.login.respond");
		goto respond;
	}
	crypto = json_string_value(json_object_get(json, "crypto"));
	if(crypto == NULL) {
		mlog("Warning: The packet did not have crypto.\n");
		build_not_found_json(sv, ct, rsp_json, "com.login.respond");
		goto respond;
	}
	if(strlen(crypto) != sizeof(hex_out)) {
		mlog("Warning: The crypto length is mistake.\n");
		if(sv->dump) mlog("%s\n", crypto);;
		build_not_found_json(sv, ct, rsp_json, "com.login.respond");
		goto respond;
	}

	ret = get_user_by_name(sv, username, &usr);
	if(ret < 0) {
		/*Can't find this user.*/
		mlog("Warning: The user '%s' did not registered.\n", username);
		build_not_found_json(sv, ct, rsp_json, "com.login.respond");
		goto respond;
	}

	if(!usr->is_seed_existed) {
		mlog("Warning: The user '%s' did out getting the seed.\n", username);
		build_not_found_json(sv, ct, rsp_json, "com.login.respond");
		user_put(sv, usr);
		goto respond;
	}

	if(usr->is_online) {
		mlog("Warning: The user '%s' is online.\n", username);
		build_not_found_json(sv, ct, rsp_json, "com.login.online.respond");
		user_put(sv, usr);
		goto respond;
	}

	sha256_init(&ctx);
	sha256_update(&ctx, usr->seed, SERVER_SEED_LENS);
	sha256_update(&ctx, usr->password, SERVER_PASSWORD_LENS);
	sha256_final(&ctx, crypto_out);
	hex_to_ascii(hex_out, crypto_out, sizeof(crypto_out));
	if(memcmp(hex_out, crypto, sizeof(hex_out))) {
		build_not_found_json(sv, ct, rsp_json, "com.login.failed.respond");
		user_put(sv, usr);
		goto respond;
	}

	alloc_new_token(sv, usr);
	json_object_set_new(rsp_json, "method", json_string("com.login.respond"));
	json_object_set_new(rsp_json, "token", json_string(usr->token));
	json_object_set_new(rsp_json, "status", json_true());
	usr->is_online = 1;
	usr->is_seed_existed = 0;
	bind_user_to_client(sv, usr, ct);
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

