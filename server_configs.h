#ifndef __SERVER_CONFIGS_H__
#define __SERVER_CONFIGS_H__

#include "mysql_connector.h"

#define SERVER_DEFAULT_PORT 1200
#define SERVER_MAX_PACKETS 4096
#define MAX_EVENTS 500
#define URANDOM_FILE_DIR "/dev/urandom"
#define SERVER_TOKEN_LENS 20
#define SERVER_SEED_LENS 20
#define SHA256_LENS (256 / 8)
#define SERVER_USERNAME_LENS 20
#define SERVER_PASSWORD_LENS SHA256_LENS
#define SERVER_DUMP_BUFFER 1
#define SERVER_STDOUT_PATH1 "/var/log/geekchat_stdout.log"
#define SERVER_STDOUT_PATH2 "/tmp/geekchat_stdout.log"
#define SERVER_STDERR_PATH1 "/var/log/geekchat_stderr.log"
#define SERVER_STDERR_PATH2 "/tmp/geekchat_stderr.log"

static struct mysql_config default_mysql_config = {
	.serverip = "127.0.0.1",
	.username = "geekchat",
	.password = "1234",
	.database_name = "geekchat",
};

#endif /*__SERVER_CONFIGS_H__*/

