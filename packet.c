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
#include "c_hashmap/hashmap.h"
#include "crc32/crc32.h"
#include "jansson/jansson.h"
#include "server_configs.h"
#include "packet.h"
#include "server.h"

int dispose_packet(struct server *sv, struct client *ct, struct raw_packet *packet)
{
	json_t *json;
	json_error_t json_err;

	json = json_loadb(packet->buffer, packet->head.packet_len, 0, &json_err);
	return 0;
}

