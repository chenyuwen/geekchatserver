#ifndef __MESSAGE_H__
#define __MESSAGE_H__

struct server;
struct user;

struct message_looper {
	struct cbtimer timer;
	struct client *ct;
	struct server *sv;
};

int send_message_to(struct server *sv, struct user *from, struct user *to,
	const char *uuid, const char *message);
int start_message_looper(struct message_looper *looper);
int stop_message_looper(struct message_looper *looper);
int is_message_looper_alive(struct message_looper *looper);

#endif /*__MESSAGE_H__*/

