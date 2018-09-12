#ifndef __SERVER_CONFIGS_H__
#define __SERVER_CONFIGS_H__

#include "mysql_connector.h"

#define SERVER_DEFAULT_PORT 1200
#define SERVER_MAX_PACKETS 4096
#define MAX_EVENTS 500

static struct mysql_config default_mysql_config = {
	.serverip = "127.0.0.1",
	.username = "geekchat",
	.password = "1234",
	.database_name = "geekchat",
};

#endif /*__SERVER_CONFIGS_H__*/

