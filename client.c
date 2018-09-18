#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "crc32/crc32.h"
#include "crypto/sha256.h"
#include "jansson/jansson.h"
#include "packet.h"
#include "hex.h"
#include "mlog.h"

#define SERVER_DEFAULT_PORT 1200
#define SERVER_DEFAULT_ADDR "149.28.70.170"
//#define SERVER_DEFAULT_ADDR "127.0.0.1"

int test_com_login_seed_request(int socketfd)
{
	unsigned char buffer[512];
	struct crc32_raw_packet *crc32_packet = (struct crc32_raw_packet *)buffer;
	struct raw_packet *packet = (void *)buffer;
	json_t *json = json_object();
	int ret = 0, i = 0;
	int raw_packet_size = 0;
	unsigned char names[][20] = {"", "lisi", "lisi", "zhangsan"};

	memset(buffer, 0, sizeof(buffer));
	json_object_set_new(json, "method", json_string("com.login.seed.request"));

	for(i=0; i<4; i++) {
		json_object_set_new(json, "username", json_string(names[i]));

		memset(buffer, 0, sizeof(buffer));
		packet->head.packet_len = json_dumpb(json, packet->buffer, 100, 0);
		packet->head.type = PACKET_TYPE_UNENCRY;

		raw_packet_size = sizeof(struct raw_packet_head) + packet->head.packet_len;
		packet->head.crc32 = crc32_classic(crc32_packet->crcdata, raw_packet_size -
			sizeof(struct crc32_raw_packet));

		printf("Write:%s\n", packet->buffer);
		ret = write(socketfd, buffer, packet->head.packet_len + sizeof(struct raw_packet_head));

		memset(buffer, 0, sizeof(buffer));
		ret = read(socketfd, buffer, 200);
		printf("Read:%s\n", packet->buffer);
		json_object_del(json, "username");
	}

	json_delete(json);
	return 0;
}

int test_com_login_request(int socketfd)
{
	unsigned char buffer[512];
	struct crc32_raw_packet *crc32_packet = (struct crc32_raw_packet *)buffer;
	struct raw_packet *packet = (void *)buffer;
	json_t *json = json_object();
	SHA256_CTX ctx;
	unsigned char crypto_out[256 / 8];
	unsigned char password[256 / 8] = "root";
	unsigned char hex_out[256 / 8 * 2 + 1] = {0};
	unsigned char token[100] = {0};
	const char *seed = NULL;
	int ret = 0, i = 0;
	int raw_packet_size = 0;
	json_error_t json_err;

	memset(buffer, 0, sizeof(buffer));
	json_object_set_new(json, "method", json_string("com.login.seed.request"));
	json_object_set_new(json, "username", json_string("root"));

	packet->head.packet_len = json_dumpb(json, packet->buffer, 100, 0);
	packet->head.type = PACKET_TYPE_UNENCRY;

	raw_packet_size = sizeof(struct raw_packet_head) + packet->head.packet_len;
	packet->head.crc32 = crc32_classic(crc32_packet->crcdata, raw_packet_size -
		sizeof(struct crc32_raw_packet));

	printf("Write:%s\n", packet->buffer);
	ret = write(socketfd, buffer, packet->head.packet_len + sizeof(struct raw_packet_head));
	json_delete(json);

	/*Read*/
	memset(buffer, 0, sizeof(buffer));
	ret = read(socketfd, buffer, 200);
	printf("Read:%s\n", packet->buffer);

	/*seed*/
	json = json_loadb(packet->buffer, packet->head.packet_len, 0, &json_err);
	seed = json_string_value(json_object_get(json,"seed"));

	sha256_init(&ctx);
	sha256_update(&ctx, seed, 32);
	sha256_update(&ctx, password, 256 / 8);
	sha256_final(&ctx, crypto_out);
	hex_to_ascii(hex_out, crypto_out, sizeof(crypto_out));
	printf("ascii %s\n", hex_out);
	printf("seed:%s len:%d\n", seed, 32);
	printf("password:%s len:%d\n", password, 256 / 8);
	json_delete(json);

	json = json_object();
	json_object_set_new(json, "method", json_string("com.login.request"));
	json_object_set_new(json, "username", json_string("root"));
	json_object_set_new(json, "crypto", json_string(hex_out));

	packet->head.packet_len = json_dumpb(json, packet->buffer, 2000, 0);
	packet->head.type = PACKET_TYPE_UNENCRY;

	raw_packet_size = sizeof(struct raw_packet_head) + packet->head.packet_len;
	packet->head.crc32 = crc32_classic(crc32_packet->crcdata, raw_packet_size -
		sizeof(struct crc32_raw_packet));

	printf("Write:%s\n", packet->buffer);
	ret = write(socketfd, buffer, packet->head.packet_len + sizeof(struct raw_packet_head));
	json_delete(json);

	memset(buffer, 0, sizeof(buffer));
	ret = read(socketfd, buffer, 1000);
	printf("Read:%s\n", packet->buffer);

	/*seed*/
	json = json_loadb(packet->buffer, packet->head.packet_len, 0, &json_err);
	strcpy(token, json_string_value(json_object_get(json,"token")));

	json = json_object();
	json_object_set_new(json, "method", json_string("com.message.sendto.request"));
	json_object_set_new(json, "message", json_string("message mmkklklkkjkk"));
	json_object_set_new(json, "token", json_string(token));
	json_object_set_new(json, "sendto", json_string("root"));

	packet->head.packet_len = json_dumpb(json, packet->buffer, 2000, 0);
	packet->head.type = PACKET_TYPE_UNENCRY;

	raw_packet_size = sizeof(struct raw_packet_head) + packet->head.packet_len;
	packet->head.crc32 = crc32_classic(crc32_packet->crcdata, raw_packet_size -
		sizeof(struct crc32_raw_packet));

	printf("Write:%s\n", packet->buffer);
	ret = write(socketfd, buffer, packet->head.packet_len + sizeof(struct raw_packet_head));
	json_delete(json);

	memset(buffer, 0, sizeof(buffer));
	ret = read(socketfd, buffer, 200);
	printf("Read:%s\n", packet->buffer);

	return 0;
}

int main(int argc, char **argv)
{
	int serverfd, ret, port = SERVER_DEFAULT_PORT;
	struct sockaddr_in serveraddr, clientaddr;
	unsigned char buffer[512];
	struct raw_packet *packet = (void *)buffer;
	struct crc32_raw_packet *crc32_packet = (struct crc32_raw_packet *)buffer;
	json_t *json;
	int raw_packet_size = 0;

	if(argc >= 2) {
		port = atoi(argv[1]);
	}
	serverfd = socket(AF_INET, SOCK_STREAM, 0);
	if(serverfd < 0) {
		perror("socket");
		return errno;
	}

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	serveraddr.sin_addr.s_addr = inet_addr(SERVER_DEFAULT_ADDR);
	memset(&(serveraddr.sin_zero), 0, sizeof(serveraddr.sin_zero));

	ret = connect(serverfd, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr));
	if(ret < 0) {
		perror("connect");
		close(serverfd);
		return errno;
	}

	printf("write\n");
	memset(buffer, 0, sizeof(buffer));
	packet->head.type = PACKET_TYPE_UNENCRY;
	json = json_object();
	json_object_set_new(json, "method", json_string("test"));
	packet->head.packet_len = json_dumpb(json, packet->buffer, 100, 0);
	printf("json:%s\n", packet->buffer);

	raw_packet_size = sizeof(struct raw_packet_head) + packet->head.packet_len;
	packet->head.crc32 = crc32_classic(crc32_packet->crcdata, raw_packet_size -
		sizeof(struct crc32_raw_packet));
	ret = write(serverfd, buffer, packet->head.packet_len + sizeof(struct raw_packet_head));

	memset(buffer, 0, sizeof(buffer));
	ret = read(serverfd, buffer, 200);
	printf("json:%s\n", packet->buffer);

	test_com_login_seed_request(serverfd);
	test_com_login_request(serverfd);
	printf("close\n");
	close(serverfd);
	json_delete(json);

	return 0;
}
