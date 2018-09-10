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

int method_hello_handler(struct server *sv, struct client *cr, json_t *json)
{
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

int call_method(struct server *sv, struct client *cr, json_t *json, const char *method)
{
	int i = 0;
	for(i=0; i<ARRAY_SIZE(methods); i++) {
		printf("%s %s\n", method, methods[i].method_name);
		if(!strncmp(method, methods[i].method_name, strlen(methods[i].method_name))) {
			methods[i].method_handler(sv, cr, json);
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

