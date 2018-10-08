#ifndef __PACKET_H__
#define __PACKET_H__

#include "c_hashmap/hashmap.h"
#include "crc32/crc32.h"
#include "jansson/jansson.h"
#include "server_configs.h"
#include "server.h"

struct client;
struct server;

#define PACKET_TYPE_UNENCRY 0x01
#define PACKET_TYPE_ENCRY 0x02
#define PACKET_TYPE_RSA 0x02

#pragma pack (1)
struct raw_packet_head {
	uint32_t crc32;
	uint16_t packet_len;
	uint16_t type;
};

struct raw_packet {
	union {
		struct {
			struct raw_packet_head head;
			char buffer[0];
		};
		char reserved[SERVER_MAX_PACKETS];
	};
};

struct crc32_raw_packet {
	uint32_t crc32;
	uint8_t crcdata[0];
};
#pragma pack ()

struct method {
	const char *name;
	int (*handler)(struct server *, struct client *, json_t *);
};

#define RAW_PACKET_BUFFER_MAX (SERVER_MAX_PACKETS - sizeof(struct raw_packet_head))

int sizeof_raw_packet(struct raw_packet *packet);
struct raw_packet *alloc_raw_packet(struct server *sv, struct client *ct);
int free_raw_packet(struct server *sv, struct client *ct,
	struct raw_packet *packet);
int respond_raw_packet(struct server *sv, struct client *ct,
	struct raw_packet *packet);
int build_simplify_json(json_t *json, const char *method, int err);
int dispatch_packet(struct server *sv, struct client *ct, struct raw_packet *packet);
int json_to_raw_packet(json_t *json, int type, struct raw_packet *packet);
int respond_raw_packet(struct server *sv, struct client *ct, struct raw_packet *packet);

#endif /*__PACKET_H__*/

