#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <mysql/mysql.h>
#include "server.h"
#include "packet.h"
#include "mysql_connector.h"

int mysql_test_connection(struct server *sv)
{
	struct mysql_config *config = sv->mysql_config;
	MYSQL_RES *result = NULL;
	int ret = 0;

	ret = mysql_query(config->mysql, "show tables");
	if(ret < 0) {
		printf("mysql_query: %s\n", mysql_error(config->mysql));
		return -mysql_errno(config->mysql);
	}

	result = mysql_use_result(config->mysql);
	if(result == NULL) {
		printf("mysql_use_result: %s\n", mysql_error(config->mysql));
		return -mysql_errno(config->mysql);
	}

	printf("The mysql is readly\n");
	mysql_free_result(result);
	return 0;
}

int init_mysql(struct server *sv)
{
	int ret = 0;
	struct mysql_config *config = sv->mysql_config;

	config->mysql = mysql_init(NULL);

	mysql_real_connect(config->mysql, config->serverip, config->username,
		config->password, config->database_name, 0, NULL, 0);
	if(-mysql_errno(config->mysql) < 0) {
		printf("mysql_real_connect: %s\n", mysql_error(config->mysql));
		return -mysql_errno(config->mysql);
	}
	return mysql_test_connection(sv);
}

int exit_mysql(struct server *sv)
{
	struct mysql_config *config = sv->mysql_config;
	mysql_close(config->mysql);
}

