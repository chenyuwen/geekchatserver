#ifndef __METHODS_H__
#define __METHODS_H__

#include "packet.h"

extern int method_hello_handler(struct server *sv, struct client *ct, json_t *json);

static struct method methods[] = {
	{
		"com.hello.request",
		method_hello_handler,
	},
};

#endif /*__METHODS_H__*/

