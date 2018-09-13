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
#include "jansson/jansson.h"
#include "packet.h"

#define SERVER_DEFAULT_PORT 1200
//#define SERVER_DEFAULT_ADDR "149.28.70.170"
#define SERVER_DEFAULT_ADDR "127.0.0.1"

int dump_buffer(const unsigned char *buffer, int len)
{
	int i = 0;
	for(i=0; i<len; i++) {
		if(buffer[i] <= 0xF) {
			printf("0x0%X ", buffer[i]);
		} else {
			printf("0x%X ", buffer[i]);
		}
		if((i % 20) == 19) {
			printf("\n");
		}
	}
	return 0;
}

int test_com_login_seed_request(int socketfd)
{
	unsigned char buffer[512];
	struct crc32_raw_packet *crc32_packet = (struct crc32_raw_packet *)buffer;
	struct raw_packet *packet = (void *)buffer;
	uint32_t crc32 = 0;
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
		crc32 = crc32_classic(crc32_packet->crcdata, raw_packet_size -
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

int main(int argc, char **argv)
{
	int serverfd, ret, port = SERVER_DEFAULT_PORT;
	uint32_t crc32 = 0;
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
	crc32 = crc32_classic(crc32_packet->crcdata, raw_packet_size -
		sizeof(struct crc32_raw_packet));
	packet->head.crc32 = htonl(crc32);
	ret = write(serverfd, buffer, packet->head.packet_len + sizeof(struct raw_packet_head));

	memset(buffer, 0, sizeof(buffer));
	ret = read(serverfd, buffer, 200);
	printf("json:%s\n", packet->buffer);

	test_com_login_seed_request(serverfd);
	printf("close\n");
	close(serverfd);
	json_delete(json);

	return 0;
}
