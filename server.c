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
#include "server.h"
#include "methods/methods.h"
#include "users.h"
#include "random_pool.h"
#include "hex.h"
#include "mlog.h"
#include "tokens.h"
#include "timer.h"

#define addr_ntoa(addr)  (inet_ntoa(((struct sockaddr_in *)(addr))->sin_addr))
int free_client_socket(struct server *sv, struct client *ct);

int client_timeout_handler(struct cbtimer *timer, void *arg)
{
	struct client *ct = container_of(timer, struct client, timer);
	struct server *sv = (void *)arg;
	free_client_socket(sv, ct);
	return 0;
}

int accept_new_client(struct server *sv)
{
	int ret = 0, flags = 0;
	struct client *ct = NULL;
	socklen_t socklen;

	ct = (void *)malloc(sizeof(struct client));
	if(ct == NULL) {
		mlog("alloc memory failed.\n");
		return errno;
	}
	memset(ct, 0, sizeof(struct client));
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
	if(hashmap_remove(sv->clients_map, ct->name) == MAP_OK) {
		mlog("failed.\n");
		return -1;
	}
	hashmap_put(sv->clients_map, ct->name, ct);
	/*TODO: unlock*/
	ct->event.events = EPOLLIN | EPOLLET | EPOLLERR;
	ct->event.data.fd = ct->fd;
	ret = epoll_ctl(sv->epollfd, EPOLL_CTL_ADD, ct->fd, &ct->event);
	if(ret < 0) {
		perror("epoll_ctl");
		return errno;
	}
	strcpy(ct->ipaddr, addr_ntoa(&ct->addr));
	ct->timer.handler = client_timeout_handler;
	ct->timer.arg = sv;
	add_timer(sv, &ct->timer, SERVER_CLIENT_UNUSED_TIMEOUT);
	mlog("New client:%s\n", ct->ipaddr);

	return 0;
}

int free_client_socket(struct server *sv, struct client *ct)
{
	int ret = 0;
	mlog("Close the client:%s\n", ct->ipaddr);
	ret = epoll_ctl(sv->epollfd, EPOLL_CTL_DEL, ct->fd, NULL);
	if(ret < 0) {
		perror("epoll_ctl");
		ret = errno;
	}

	/*TODO: lock*/
	hashmap_remove(sv->clients_map, ct->name);
	/*TODO: unlock*/
	if(is_user_bind(ct->usr)) {
		unbind_user(ct->usr);
	}
	if(is_timer_effective(sv, &ct->timer)) {
		del_timer(sv, &ct->timer);
	}
	close(ct->fd);
	free(ct);
	return ret;
}

int try_make_net_packet(struct server *sv, struct client *ct)
{
	int i = 0, raw_packet_size = 0;
	uint32_t crc32 = 0;
	struct raw_packet *packet = (struct raw_packet *)ct->buffer;
	struct crc32_raw_packet *crc32_packet = (struct crc32_raw_packet *)ct->buffer;

	if(ct->buffer_offset < sizeof(struct raw_packet_head)) {
		mlog("It is not a packet.\n");
		if(sv->dump) dump_buffer((void *)packet, ct->buffer_offset);
		return 0;
	}

	raw_packet_size = sizeof_raw_packet(packet);
	if(ct->buffer_offset < raw_packet_size) {
		mlog("It is not a packet.\n");
		if(sv->dump) dump_buffer((void *)packet, ct->buffer_offset);
		return 0;
	}

	crc32 = crc32_classic(crc32_packet->crcdata, raw_packet_size -
		sizeof(struct crc32_raw_packet));
	if(crc32 != packet->head.crc32) {
		mlog("CRC error.\n");
		if(sv->dump) dump_buffer((void *)&crc32, sizeof(crc32));
		if(sv->dump) dump_buffer((void *)packet, raw_packet_size);
		return -1;
	}
	dispatch_packet(sv, ct, packet);

	ct->buffer_offset -= raw_packet_size;
	if(ct->buffer_offset > 0) {
		memcpy(ct->buffer, ct->buffer + raw_packet_size, ct->buffer_offset);
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
	ret = hashmap_get(sv->clients_map, name, (any_t *)&ct);
	/*TODO:unlock*/
	if(ret != MAP_OK) {
		mlog("Error\n");
		return -1;
	}

	if((event->events & EPOLLHUP) || (event->events & EPOLLERR)) {
		return free_client_socket(sv, ct);
	} else if(event->events & EPOLLIN) {
		ret = read(ct->fd, ct->buffer + ct->buffer_offset, sizeof(ct->buffer) -
			ct->buffer_offset - sizeof(struct raw_packet_head));
		if(ret <= 0) {
			return free_client_socket(sv, ct);
		}

		ct->buffer_offset += ret;
		ret = try_make_net_packet(sv, ct);
		kick_timer(sv, &ct->timer);
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
	sv->clients_map = hashmap_new();
	sv->methods_map = hashmap_new();
	sv->mysql_config = &default_mysql_config;
	sv->dump = SERVER_DUMP_BUFFER;
	init_timer(sv);
//	init_mlog(sv);
	init_token(sv);
	init_methods_maps(sv);
	init_users_map(sv);
	init_mysql(sv);
	ret = init_random_pool(&sv->random);
	if(ret < 0) {
		return ret;
	}
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
		ret = epoll_wait(sv->epollfd, sv->eventlist, MAX_EVENTS, 1000);
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
		handle_timer_list(sv);
	}

	return 0;
}
