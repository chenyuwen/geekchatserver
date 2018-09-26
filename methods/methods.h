#ifndef __METHODS_H__
#define __METHODS_H__

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
#endif

extern int method_com_hello_request(struct server *sv, struct client *ct, json_t *json);
extern int method_com_login_seed_request(struct server *sv, struct client *ct, json_t *json);
extern int method_com_login_request(struct server *sv, struct client *ct, json_t *json);
extern int method_com_message_sendto_request(struct server *sv, struct client *ct, json_t *json);
extern int method_com_friends_list_request(struct server *sv, struct client *ct, json_t *json);
extern int method_com_logout_bytoken_request(struct server *sv, struct client *ct, json_t *json);
extern int method_com_friends_delete_request(struct server *sv, struct client *ct, json_t *json);
extern int method_com_friends_add_request(struct server *sv, struct client *ct, json_t *json);
extern int method_com_friends_info_request(struct server *sv, struct client *ct, json_t *json);
extern int method_com_heartbeat_request(struct server *sv, struct client *ct, json_t *json);
extern int method_com_register_request(struct server *sv, struct client *ct, json_t *json);
extern int method_com_user_changeinfo_request(struct server *sv, struct client *ct, json_t *json);
extern int method_com_user_info_request(struct server *sv, struct client *ct, json_t *json);
extern int method_com_user_password_request(struct server *sv, struct client *ct, json_t *json);
extern int method_com_message_recv_respond(struct server *sv, struct client *ct, json_t *json);

static struct method methods[] = {
	{
		"com.hello.request",
		method_com_hello_request,
	},
	{
		"com.login.seed.request",
		method_com_login_seed_request,
	},
	{
		"com.login.request",
		method_com_login_request,
	},
	{
		"com.message.sendto.request",
		method_com_message_sendto_request,
	},
	{
		"com.friends.list.request",
		method_com_friends_list_request,
	},
	{
		"com.logout.bytoken.request",
		method_com_logout_bytoken_request,
	},
	{
		"com.friends.delete.request",
		method_com_friends_delete_request,
	},
	{
		"com.friends.add.request",
		method_com_friends_add_request,
	},
	{
		"com.friends.info.request",
		method_com_friends_info_request,
	},
	{
		"com.heartbeat.request",
		method_com_heartbeat_request,
	},
	{
		"com.register.request",
		method_com_register_request,
	},
	{
		"com.user.changeinfo.request",
		method_com_user_changeinfo_request,
	},
	{
		"com.user.info.request",
		method_com_user_info_request,
	},
	{
		"com.user.password.request",
		method_com_user_password_request,
	},
	{
		"com.message.recv.respond",
		method_com_message_recv_respond,
	},
};

static int init_methods_maps(struct server *sv)
{
	int i = 0, ret;
	for(i=0; i<ARRAY_SIZE(methods); i++) {
		ret = hashmap_put(sv->methods_map, (char *)methods[i].name, (any_t)methods[i].handler);
	}
	return 0;
}

#endif /*__METHODS_H__*/

