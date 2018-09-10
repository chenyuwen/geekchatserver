#ifndef __SERVER_H__
#define __SERVER_H__

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
#include "server_configs.h"
#include "packet.h"

struct timeout {
	int alive_time;
};

struct client {
	struct epoll_event event;
	char name[20], ipaddr[20];
	int fd;
	struct sockaddr addr;
	char *token;
	struct timeout timeout;

	int buffer_offset;
	char buffer[SERVER_MAX_PACKETS];
	char respond[SERVER_MAX_PACKETS];
};

struct server {
	int serverfd, epollfd, stop_server;
	struct epoll_event event, eventlist[MAX_EVENTS];
	struct sockaddr_in serveraddr;

	map_t clients_map;
	map_t methods_map;
};

#endif /*__SERVER_H__*/

