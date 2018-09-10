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

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
#endif

struct method {
	char *method_name;
	int (*method_handler)(struct server *, struct client *, json_t *);
};

int json_to_raw_packet(struct raw_packet *packet, json_t *json, int type)
{
	uint32_t crc32;
	struct crc32_raw_packet *crc32_packet = (struct crc32_raw_packet *)packet;

	packet->head.type = type;
	packet->head.packet_len = json_dumpb(json, packet->buffer, 100, 0);
	crc32 = crc32_classic(&crc32_packet->crcdata, packet->head.packet_len);
	packet->head.crc32 = htonl(crc32);
	return 0;
}

int method_hello_handler(struct server *sv, struct client *ct, json_t *json)
{
	json_error_t json_err;
	json_t *rjson = json_object();
	struct raw_packet *packet = (void *)ct->respond;

	json = json_object();
	json_object_set_new(rjson, "method", json_string("com.hello.respond"));
	json_object_set_new(rjson, "status", json_true());
	json_to_raw_packet(packet, rjson, PACKET_TYPE_UNENCRY);

	write(ct->fd, (void *)packet, sizeof(struct raw_packet_head) + packet->head.packet_len);

	/*TODO: free json*/
	printf("hello\n");
	return 0;
}

struct method methods[] = {
	{
		"com.hello.request",
		method_hello_handler,
	},

	{
	},
};

int call_method(struct server *sv, struct client *ct, json_t *json, const char *method)
{
	int i = 0;
	for(i=0; i<ARRAY_SIZE(methods); i++) {
		printf("%s %s\n", method, methods[i].method_name);
		if(!strncmp(method, methods[i].method_name, strlen(methods[i].method_name))) {
			methods[i].method_handler(sv, ct, json);
			break;
		}
	}
	return 0;
}

int dispose_packet(struct server *sv, struct client *ct, struct raw_packet *packet)
{
	json_t *json, *method_json;
	const char *tmp = NULL;
	json_error_t json_err;

	printf("json:%s len:%d\n", packet->buffer, packet->head.packet_len);
	json = json_loadb(packet->buffer, strlen(packet->buffer), 0, &json_err);
	if(json == NULL) {
		printf("json_loadb failed.\n");
		return -1;
	}

	method_json = json_object_get(json, "method");
	tmp = json_string_value(method_json);
	if(tmp == NULL) {
		printf("methos is null.\n");
		return -1;
	}
	call_method(sv, ct, json, tmp);
	return 0;
}

