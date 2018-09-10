#ifndef __METHODS_H__
#define __METHODS_H__

#include "packet.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
#endif

extern int method_hello_handler(struct server *sv, struct client *ct, json_t *json);

static struct method methods[] = {
	{
		"com.hello.request",
		method_hello_handler,
	},
};

static int init_methods_maps(struct server *sv)
{
	int i = 0, ret;
	for(i=0; i<ARRAY_SIZE(methods); i++) {
		ret = hashmap_put(sv->methods_map, (char *)methods->name, (any_t)methods->handler);
	}
	return 0;
}

#endif /*__METHODS_H__*/

