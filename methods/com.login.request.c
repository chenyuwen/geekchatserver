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
#include "../server_errno.h"
#include "methods.h"

#define THIS_METHOD_RESPOND_NAME "com.login.respond"

int method_com_login_request(struct server *sv, struct client *ct, json_t *json)
{
	json_error_t json_err;
	json_t *rsp_json = json_object();
	struct raw_packet *packet = alloc_raw_packet(sv, ct);
	const char *username = NULL, *crypto = NULL;
	unsigned char crypto_out[SHA256_LENS];
	unsigned char hex_out[SHA256_LENS * 2];
	SHA256_CTX ctx;
	struct user *usr;
	int ret = 0, res_ret = SERR_SUCCESS;

	username = json_string_value(json_object_get(json, "username"));
	if(username == NULL) {
		mlog("Warning: The packet did not have username.\n");
		res_ret = -SERR_ARG;
		goto respond;
	}
	crypto = json_string_value(json_object_get(json, "crypto"));
	if(crypto == NULL) {
		mlog("Warning: The packet did not have crypto.\n");
		res_ret = -SERR_ARG;
		goto respond;
	}
	if(strlen(crypto) != sizeof(hex_out)) {
		mlog("Warning: The crypto length is mistake.\n");
		if(sv->dump) mlog("%s\n", crypto);;
		res_ret = -SERR_ARG;
		goto respond;
	}

	ret = get_user_by_name(sv, username, &usr);
	if(ret < 0) {
		/*Can't find this user.*/
		mlog("Warning: The user '%s' did not registered.\n", username);
		res_ret = -SERR_PWD_ERR;
		goto respond;
	}

	if(!usr->is_seed_existed) {
		mlog("Warning: The user '%s' did out getting the seed.\n", username);
		user_put(sv, usr);
		res_ret = -SERR_PWD_ERR;
		goto respond;
	}

	sha256_init(&ctx);
	sha256_update(&ctx, usr->seed, SERVER_SEED_LENS);
	sha256_update(&ctx, usr->password, SERVER_PASSWORD_LENS);
	sha256_final(&ctx, crypto_out);
	hex_to_ascii(hex_out, crypto_out, sizeof(crypto_out));
	if(memcmp(hex_out, crypto, sizeof(hex_out))) {
		user_put(sv, usr);
		res_ret = -SERR_PWD_ERR;
		goto respond;
	}

	if(usr->is_online) {
		mlog("Warning: The user '%s' is online.\n", username);
		user_put(sv, usr);
		res_ret = -SERR_IS_ONLINE;
		goto respond;
	}

	alloc_new_token(sv, usr);
	json_object_set_new(rsp_json, "token", json_string(usr->token));
	usr->is_online = 1;
	usr->is_seed_existed = 0;
	bind_user_to_client(sv, usr, ct);
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

