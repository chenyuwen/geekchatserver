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

#define addr_ntoa(addr)  (inet_ntoa(((struct sockaddr_in *)(addr))->sin_addr))

struct timeout {
	int alive_time;
};

struct client {
	struct epoll_event event;
	char name[20];
	int fd;
	struct sockaddr addr;
	char *token;
	struct timeout timeout;

	int buffer_offset;
	struct raw_packet packet;
	char buffer[SERVER_MAX_PACKETS];
};

struct server {
	int serverfd, epollfd, stop_server;
	struct epoll_event event, eventlist[MAX_EVENTS];
	struct sockaddr_in serveraddr;

	map_t client_map;
	map_t handler_map;
};

int accept_new_client(struct server *sv)
{
	int ret = 0, flags = 0;
	struct client *ct = NULL;
	socklen_t socklen;

	ct = (void *)malloc(sizeof(struct client));
	memset(ct, 0, sizeof(*ct));
	socklen = sizeof(struct sockaddr);
	ct->fd = accept(sv->serverfd, &ct->addr, &socklen);
	if(ct->fd < 0) {
		perror("accept");
		return errno;
	}

	/*O_NONBLOCK*/
	flags = fcntl(ct->fd, F_GETFL, 0);
	if(flags < 0) {
		perror("fcntl\n");
		return errno;
	}
	ret = fcntl(ct->fd, F_SETFL, flags | O_NONBLOCK);
	if(ret < 0) {
		perror("fcntl\n");
		return errno;
	}

	snprintf(ct->name, sizeof(ct->name), "%d", ct->fd);
	/*TODO: lock*/
	if(hashmap_remove(sv->client_map, ct->name) == MAP_OK) {
		printf("failed.\n");
		return -1;
	}
	hashmap_put(sv->client_map, ct->name, ct);
	/*TODO: unlock*/
	ct->event.events = EPOLLIN | EPOLLET | EPOLLERR;
	ct->event.data.fd = ct->fd;
	ret = epoll_ctl(sv->epollfd, EPOLL_CTL_ADD, ct->fd, &ct->event);
	if(ret < 0) {
		perror("epoll_ctl");
		return errno;
	}
	printf("New client:%s\n", addr_ntoa(&ct->addr));

	return 0;
}

int free_client_socket(struct server *sv, struct client *ct)
{
	int ret = 0;
	printf("Close the client:%s\n", addr_ntoa(&ct->addr));
	ret = epoll_ctl(sv->epollfd, EPOLL_CTL_DEL, ct->fd, NULL);
	if(ret < 0) {
		perror("epoll_ctl");
		ret = errno;
	}

	/*TODO: lock*/
	hashmap_remove(sv->client_map, ct->name);
	/*TODO: unlock*/
	close(ct->fd);
	free(ct);
	return ret;
}

int try_make_net_packet(struct server *sv, struct client *ct)
{
	int i = 0, packet_size = 0;
	uint32_t crc32 = 0;
	struct raw_packet *packet = (struct raw_packet *)ct->buffer;
	struct crc32_raw_packet *crc32_packet = (struct crc32_raw_packet *)ct->buffer;

	if(ct->buffer_offset < sizeof(struct raw_packet_head)) {
		printf("It is not a packet.\n");
		return 0;
	}

	packet_size = packet->head.packet_len + sizeof(struct raw_packet_head);
	if(ct->buffer_offset < packet_size) {
		printf("It is not a packet.\n");
		return 0;
	}

	crc32 = crc32_classic(&crc32_packet->crcdata, packet->head.packet_len);
	if(crc32 != ntohl(packet->head.crc32)) {
		printf("CRC error.\n");
		return -1;
	}
	printf("CRC GOOD.\n");
	/*TODO: */

	ct->buffer_offset -= packet_size;
	printf("packetsize : %d offset:%d\n", packet_size, ct->buffer_offset);
	if(ct->buffer_offset > 0) {
		memcpy(ct->buffer, ct->buffer + packet_size, ct->buffer_offset);
		return try_make_net_packet(sv, ct);
	}

	return 0;
}

int client_socket(struct server *sv, struct epoll_event *event)
{
	int ret = 0;
	char name[20];
	struct client *ct = NULL;

	snprintf(name, sizeof(name), "%d", event->data.fd);
	/*TODO:lock*/
	ret = hashmap_get(sv->client_map, name, (any_t *)&ct);
	/*TODO:unlock*/
	if(ret != MAP_OK) {
		printf("Error\n");
		return -1;
	}

	if((event->events & EPOLLHUP) || (event->events & EPOLLERR)) {
		return free_client_socket(sv, ct);
	} else if(event->events & EPOLLIN) {
		ret = read(ct->fd, ct->buffer + ct->buffer_offset, sizeof(ct->buffer) -
			ct->buffer_offset);
		if(ret <= 0) {
			return free_client_socket(sv, ct);
		}

		ct->buffer_offset += ret;
		ret = try_make_net_packet(sv, ct);
		if(ret < 0) {
			return free_client_socket(sv, ct);
		}
	}
	return 0;
}

int main(int argc, char **argv)
{
	int ret = 0, i = 0, port = SERVER_DEFAULT_PORT, flags = 0;
	struct server *sv = (void *)malloc(sizeof(struct server));

	if(argc >= 2) {
		port = atoi((void *)argv[1]);
	}

	memset(sv, 0, sizeof(*sv));
	sv->client_map = hashmap_new();
	sv->serverfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sv->serverfd < 0) {
		perror("socket");
		return errno;
	}

	/*O_NONBLOCK*/
	flags = fcntl(sv->serverfd, F_GETFL, 0);
	if(flags < 0) {
		perror("fcntl\n");
		return errno;
	}
	ret = fcntl(sv->serverfd, F_SETFL, flags | O_NONBLOCK);
	if(ret < 0) {
		perror("fcntl\n");
		return errno;
	}

	sv->serveraddr.sin_family = AF_INET;
	sv->serveraddr.sin_port = htons(port);
	sv->serveraddr.sin_addr.s_addr = htons(INADDR_ANY);
	memset(&(sv->serveraddr.sin_zero), 0, sizeof(sv->serveraddr.sin_zero));

	ret = bind(sv->serverfd, (struct sockaddr *)&sv->serveraddr, sizeof(struct sockaddr));
	if(ret < 0) {
		perror("bind");
		close(sv->serverfd);
		return errno;
	}

	listen(sv->serverfd, 4);

	sv->epollfd = epoll_create(MAX_EVENTS);
	sv->event.events = EPOLLIN | EPOLLET;
	sv->event.data.fd = sv->serverfd;
	ret = epoll_ctl(sv->epollfd, EPOLL_CTL_ADD, sv->serverfd, &sv->event);
	if(ret < 0) {
		perror("epoll_ctl");
		close(sv->serverfd);
		return errno;
	}

	while(!sv->stop_server) {
		ret = epoll_wait(sv->epollfd, sv->eventlist, MAX_EVENTS, 3000);
		if(ret < 0) {
			perror("epoll_wait");
			return errno;
		}

		for(i=0; i<ret; i++) {
			if(sv->eventlist[i].data.fd == sv->serverfd) {
				accept_new_client(sv);
			} else {
				client_socket(sv, &sv->eventlist[i]);
			}
		}
	}

	return 0;
}
