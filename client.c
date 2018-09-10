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
#include "packet.h"

#define SERVER_DEFAULT_PORT 1200
//#define SERVER_DEFAULT_ADDR "149.28.70.170"
#define SERVER_DEFAULT_ADDR "127.0.0.1"

/*
struct client {
	int fd;
	struct sockaddr addr;
	char *token;
};
*/

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

int main(int argc, char **argv)
{
	int serverfd, clientfd, ret, port = SERVER_DEFAULT_PORT;
	uint32_t crc32 = 0;
	struct sockaddr_in serveraddr, clientaddr;
	unsigned char buffer[512];
	struct raw_packet *packet = (void *)buffer;
	struct crc32_raw_packet *crc32_packet = (struct crc32_raw_packet *)buffer;

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
	packet->head.packet_len = 100;
	packet->head.type = PACKET_TYPE_UNENCRY;
	memset(packet->buffer, '1', 99);
	crc32 = crc32_classic(&crc32_packet->crcdata, packet->head.packet_len);
	packet->head.crc32 = htonl(crc32);
	memcpy(buffer + 108, buffer, 108);
	ret = write(serverfd, buffer, 100);
	ret = write(serverfd, buffer + 100, sizeof(struct raw_packet_head));
	ret = write(serverfd, buffer, 108);
	ret = write(serverfd, buffer, 108);
	ret = write(serverfd, buffer, 108);
	ret = write(serverfd, buffer, 108);
	printf("write 2\n");
	dump_buffer(buffer, 108 * 2);
	ret = write(serverfd, buffer, 108 * 2);
	printf("write 2 - 100\n");
	ret = write(serverfd, buffer, 108 * 2 - 100);
	ret = write(serverfd, buffer + 108 * 2 - 100, 100);
	/**/
	ret = write(serverfd, buffer, 108);
	printf("close\n");
	close(serverfd);

	return 0;
}
