#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include "crc32/crc32.h"
#include "crypto/sha256.h"
#include "jansson/jansson.h"
#include "packet.h"
#include "hex.h"
#include "mlog.h"

#define SERVER_DEFAULT_PORT 1200
#define SERVER_DEFAULT_ADDR "45.32.82.181"
//#define SERVER_DEFAULT_ADDR "127.0.0.1"
#define CLIENT_DUMP 1
#define STDIN 0

struct client_struct {
	unsigned char password[256 / 8];
	unsigned char username[21];
	unsigned char token[100];
	int fd;

	int recv_offset;
	unsigned char recv_buffer[4096], send_buffer[4096];
};

int recv_json_object(struct client_struct *ct, json_t **json)
{
	struct raw_packet *recv_packet = (void *)ct->recv_buffer;
	json_t *recv_json = NULL;
	json_error_t json_err;
	int ret = 0, packet_size = 0;

	if(CLIENT_DUMP) memset(ct->recv_buffer, 0, sizeof(ct->recv_buffer));
	ret = read(ct->fd, ct->recv_buffer, sizeof(ct->recv_buffer));
	if(ret < 0) {
		perror("read\n");
		goto out;
	}
	ct->recv_offset += ret;

	if(CLIENT_DUMP) printf("%s\n", recv_packet->buffer);
	recv_json = json_loadb(recv_packet->buffer, recv_packet->head.packet_len, 0, &json_err);
	if(recv_json == NULL) {
		printf("It is not a json.\n");
		ret = -1;
		goto out;
	}
	*json = recv_json;
	packet_size = sizeof(struct raw_packet_head) + recv_packet->head.packet_len;
	ct->recv_offset -= packet_size;

out:
	return ret;
}

int recv_buffer(struct client_struct *ct)
{
	struct raw_packet *recv_packet = (void *)ct->recv_buffer;
	int ret = 0;

	ret = read(ct->fd, ct->recv_buffer, sizeof(ct->recv_buffer));
	if(ret < 0) {
		perror("read\n");
		goto out;
	}
	ct->recv_offset += ret;

out:
	return ret;
}

int fetch_json(struct client_struct *ct, json_t **json)
{
	struct raw_packet *recv_packet = (void *)ct->recv_buffer;
	json_t *recv_json = NULL;
	json_error_t json_err;
	int ret = 0, packet_size = 0;

	if(ct->recv_offset < sizeof(struct raw_packet_head)) {
		return -1;
	}

	packet_size = sizeof(struct raw_packet_head) + recv_packet->head.packet_len;
	if(ct->recv_offset < packet_size) {
		return -1;
	}

	recv_json = json_loadb(recv_packet->buffer, recv_packet->head.packet_len, 0, &json_err);
	if(recv_json == NULL) {
		printf("It is not a json.\n");
		ret = -1;
		goto out;
	}
	*json = recv_json;
	ct->recv_offset -= packet_size;
	memcpy(ct->recv_buffer, ct->recv_buffer + packet_size, ct->recv_offset);

out:
	return 0;
}

int send_json_object(struct client_struct *ct, json_t *json)
{
	struct raw_packet *send_packet = (void *)ct->send_buffer;
	struct crc32_raw_packet *crc32_send_packet = (void *)ct->send_buffer;
	int ret = 0, raw_packet_size = 0;

	if(CLIENT_DUMP) memset(ct->send_buffer, 0, sizeof(ct->send_buffer));
	send_packet->head.packet_len = json_dumpb(json, send_packet->buffer, sizeof(ct->send_buffer) -
		sizeof(struct raw_packet_head), 0);
	send_packet->head.type = PACKET_TYPE_UNENCRY;

	raw_packet_size = sizeof(struct raw_packet_head) + send_packet->head.packet_len;
	send_packet->head.crc32 = crc32_classic(crc32_send_packet->crcdata, raw_packet_size -
		sizeof(struct crc32_raw_packet));

	if(CLIENT_DUMP) printf("%s\n", send_packet->buffer);
	ret = write(ct->fd, ct->send_buffer, send_packet->head.packet_len + sizeof(struct raw_packet_head));
	if(ret < 0) {
		perror("write");
	}
	return ret;
}

int login_to_server(struct client_struct *ct)
{
	json_t *recv_json = NULL;
	json_t *send_json = json_object();
	int ret = 0;
	unsigned char crypto_out[256 / 8];
	unsigned char hex_out[256 / 8 * 2 + 1] = {0};
	const char *recv_seed = NULL, *recv_token = NULL;
	SHA256_CTX ctx;

	/*Seed send*/
	memset(ct->send_buffer, 0, sizeof(ct->send_buffer));
	json_object_set_new(send_json, "method", json_string("com.login.seed.request"));
	json_object_set_new(send_json, "username", json_string(ct->username));
	send_json_object(ct, send_json);
	json_delete(send_json);

	/*Seed recv*/
	if(recv_json_object(ct, &recv_json) < 0) {
		ret = -1;
		goto out;
	}
	recv_seed = json_string_value(json_object_get(recv_json, "seed"));
	if(recv_seed == NULL) {
		printf("The seed attr did not in json\n");
		ret = -1;
		goto out;
	}

	/*Login send*/
	sha256_init(&ctx);
	sha256_update(&ctx, recv_seed, 32);
	sha256_update(&ctx, ct->password, 256 / 8);
	sha256_final(&ctx, crypto_out);
	hex_to_ascii(hex_out, crypto_out, sizeof(crypto_out));

	send_json = json_object();
	json_object_set_new(send_json, "method", json_string("com.login.request"));
	json_object_set_new(send_json, "crypto", json_string(hex_out));
	json_object_set_new(send_json, "username", json_string(ct->username));
	send_json_object(ct, send_json);
	json_delete(send_json);

	/*Login recv*/
	json_delete(recv_json);
	if(recv_json_object(ct, &recv_json) < 0) {
		ret = -1;
		goto out;
	}

	recv_token = json_string_value(json_object_get(recv_json, "token"));
	if(recv_token == NULL) {
		printf("The seed attr did not in json\n");
		ret = -1;
		goto out;
	}
	strncpy(ct->token, recv_token, sizeof(ct->token));
	json_delete(recv_json);

out:
	return ret;
}

int recv_stdin(struct client_struct *ct)
{
	json_t *json = json_object();
	unsigned char sendto[30], message[100];
	scanf("%s%s", sendto, message);
	json_object_set_new(json, "method", json_string("com.message.sendto.request"));
	json_object_set_new(json, "message", json_string(message));
	json_object_set_new(json, "token", json_string(ct->token));
	json_object_set_new(json, "sendto", json_string(sendto));

	send_json_object(ct, json);
	json_delete(json);
	return 0;
}

int recv_message(struct client_struct *ct)
{
	json_t *json = json_object();
	const char *from, *message;
	int ret = 0;

	if(recv_buffer(ct) < 0) {
		ret = -1;
		goto out;
	}

	while(fetch_json(ct, &json) >= 0) {
		from = json_string_value(json_object_get(json, "from"));
		message = json_string_value(json_object_get(json, "message"));
		if(from == NULL || message == NULL) {
			json_delete(json);
			goto out;
		}

		printf("%s: %s\n", from, message);
		json_delete(json);
	}

out:
	return ret;
}

int list_my_friends(struct client_struct *ct)
{
	json_t *json = json_object();
	int len = 0, i = 0;
	json_t *obj, *array;

	json_object_set_new(json, "method", json_string("com.friends.list.request"));
	json_object_set_new(json, "token", json_string(ct->token));
	send_json_object(ct, json);
	json_delete(json);

	recv_json_object(ct, &json);
	array = json_object_get(json, "friends");
	printf("Friends:\n");
	json_array_foreach(array, i, obj) {
		printf("%s\t", json_string_value(obj));
	}
	printf("\n");
	json_delete(json);
	return 0;
}

int main(int argc, char **argv)
{
	int ret, i = 0, port = SERVER_DEFAULT_PORT;
	struct sockaddr_in serveraddr;
	struct client_struct ct;
	int epollfd = 0, thread_loop = 1;
	struct epoll_event event, eventlist[MAX_EVENTS];

	if(argc >= 2) {
		port = atoi(argv[1]);
	}
	ct.recv_offset = 0;
	ct.fd = socket(AF_INET, SOCK_STREAM, 0);
	if(ct.fd < 0) {
		perror("socket");
		return errno;
	}

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	serveraddr.sin_addr.s_addr = inet_addr(SERVER_DEFAULT_ADDR);
	memset(&(serveraddr.sin_zero), 0, sizeof(serveraddr.sin_zero));

	ret = connect(ct.fd, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr));
	if(ret < 0) {
		perror("connect");
		close(ct.fd);
		return errno;
	}
	strcpy(ct.username, "root");
	strcpy(ct.password, "root");

	printf("Enter username:");
	scanf("%s", ct.username);

	printf("Enter password:");
	scanf("%s", ct.password);

	if(login_to_server(&ct) < 0) {
		goto out;
	}
	list_my_friends(&ct);

	epollfd = epoll_create(MAX_EVENTS);
	event.events = EPOLLIN | EPOLLET;
	event.data.fd = ct.fd;
	ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, ct.fd, &event);
	if(ret < 0) {
		return 0;
	}

	event.events = EPOLLIN | EPOLLET;
	event.data.fd = STDIN;
	ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, STDIN, &event);
	if(ret < 0) {
		return 0;
	}

	while(thread_loop) {
		ret = epoll_wait(epollfd, eventlist, MAX_EVENTS, 3000);
		if(ret < 0) {
			perror("epoll_wait");
			return 0;
		}
		for(i=0; i<ret; i++) {
			if(eventlist[i].data.fd == ct.fd) {
				if(recv_message(&ct) < 0) {
					printf("The socket was close.\n");
					goto exit;
				}
			} else if(eventlist[i].data.fd == STDIN) {
				recv_stdin(&ct);
			}
		}
	}

exit:
	close(epollfd);

out:
	close(ct.fd);
	return 0;
}
