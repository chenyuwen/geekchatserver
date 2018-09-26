#include <string.h>
#include <mysql/mysql.h>
#include "users.h"
#include "jansson/jansson.h"
#include "server.h"
#include "users.h"
#include "mlog.h"
#include "messages.h"

int send_message_to(struct server *sv, struct user *from, struct user *to,
	const char *uuid, const char *message)
{
	json_t *rsp_json = json_object();
	struct raw_packet *packet = malloc_raw_packet(sv, to->client);
	int ret = 0;

	json_object_set_new(rsp_json, "method", json_string("com.message.recv.request"));
	json_object_set_new(rsp_json, "status", json_true());
	json_object_set_new(rsp_json, "from", json_string(from->username));
	json_object_set_new(rsp_json, "token", json_string(to->token));
	json_object_set_new(rsp_json, "uuid", json_string(uuid));
	json_object_set_new(rsp_json, "message", json_string(message));

respond:
	json_to_raw_packet(rsp_json, PACKET_TYPE_UNENCRY, packet);
	respond_raw_packet(sv, to->client, packet);

out:
	/*free json*/
	json_delete(rsp_json);
	free_raw_packet(sv, to->client, packet);
}

int get_message_from_mysql(struct server *sv, struct user *to_usr, MYSQL_RES **result)
{
	struct mysql_config *config = sv->mysql_config;
	MYSQL_RES *tmp = NULL;
	MYSQL_ROW row = NULL;
	int ret = 0;
	char query[200];

	snprintf(query, sizeof(query),"select uuid, from_user, message from messages" \
		" where to_user = '%s' and have_read = '0'", to_usr->username);
	if(sv->dump) mlog("%s\n", query);
	ret = mysql_real_query(config->mysql, query, strlen(query));
	if(ret < 0) {
		mlog("mysql_query: %s\n", mysql_error(config->mysql));
		return -mysql_errno(config->mysql);
	}

	tmp = mysql_store_result(config->mysql);
	if(tmp == NULL) {
		mlog("mysql_store_result: %s\n", mysql_error(config->mysql));
		return -mysql_errno(config->mysql);
	}

	if(mysql_num_rows(tmp) <= 0) {
		ret = -1;
		mysql_free_result(tmp);
		goto out;
	}
	*result = tmp;

out:
	return ret;
}

int send_message_handler(struct cbtimer *timer, void *arg)
{
	struct message_looper *looper = (void *)arg;
	MYSQL_RES *result = NULL;
	MYSQL_ROW row = NULL;
	struct user *from_usr = NULL;
	int ret = 0;
	int i = 0;

	ret = get_message_from_mysql(looper->sv, looper->ct->usr, &result);
	if(ret < 0 || result == NULL) {
		goto out;
	}

	while(row = mysql_fetch_row(result)) {
		ret = get_user_by_name(looper->sv, row[1], &from_usr);
		if(ret < 0 || from_usr == NULL) {
			continue;
		}

		send_message_to(looper->sv, from_usr, looper->ct->usr,
			(const char *)row[0], (char *)row[2]);

		user_put(looper->sv, from_usr);
	}
	mysql_free_result(result);

out:
	return 0;
}

int stop_message_looper(struct message_looper *looper)
{
	del_timer(looper->sv, &looper->timer);
}

int start_message_looper(struct message_looper *looper)
{
	looper->timer.handler = send_message_handler;
	looper->timer.arg = looper;
	add_timer(looper->sv, &looper->timer, 0);
}

int is_message_looper_alive(struct message_looper *looper)
{
	if(looper->sv == NULL) {
		return 0;
	}
	return is_timer_effective(looper->sv, &looper->timer);
}

