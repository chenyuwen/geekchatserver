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
#include "methods.h"

int json_to_raw_packet(struct raw_packet *packet, json_t *json, int type)
{
	uint32_t crc32;
	struct crc32_raw_packet *crc32_packet = (struct crc32_raw_packet *)packet;

	packet->head.type = type;
	packet->head.packet_len = json_dumpb(json, packet->buffer, RAW_PACKET_BUFFER_MAX, 0);
	crc32 = crc32_classic(&crc32_packet->crcdata, packet->head.packet_len);
	packet->head.crc32 = htonl(crc32);
	return 0;
}

int call_method(struct server *sv, struct client *ct, json_t *json, const char *method)
{
	int i = 0;
	int (*handler)(struct server *, struct client *, json_t *);

	hashmap_get(sv->methods_map, (char *)method, (any_t *)&handler);
	if(handler == NULL) {
		return -1;
	}
	return handler(sv, ct, json);
}

int dispose_packet(struct server *sv, struct client *ct, struct raw_packet *packet)
{
	json_t *json, *method_json;
	const char *tmp = NULL;
	json_error_t json_err;
	int ret = 0;

	json = json_loadb(packet->buffer, strlen(packet->buffer), 0, &json_err);
	if(json == NULL) {
		printf("json_loadb failed.\n");
		return -1;
	}

	method_json = json_object_get(json, "method");
	tmp = json_string_value(method_json);
	if(tmp == NULL) {
		printf("the methods is null.\n");
		return -1;
	}
	printf("From %s:%s\n", ct->ipaddr, packet->buffer);
	ret = call_method(sv, ct, json, tmp);
	json_delete(json);
	return ret;
}

