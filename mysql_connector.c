#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#include <string.h>
#include "server.h"
#include "packet.h"
#include "mysql_connector.h"
#include "mlog.h"

int mysql_test_connection(struct server *sv)
{
	struct mysql_config *config = sv->mysql_config;
	MYSQL_RES *result = NULL;
	int ret = 0;

	ret = mysql_query(config->mysql, "show tables");
	if(ret < 0) {
		mlog("mysql_query: %s\n", mysql_error(config->mysql));
		return -mysql_errno(config->mysql);
	}

	result = mysql_use_result(config->mysql);
	if(result == NULL) {
		mlog("mysql_use_result: %s\n", mysql_error(config->mysql));
		return -mysql_errno(config->mysql);
	}

	mlog("The mysql is ready\n");
	mysql_free_result(result);
	return 0;
}

int mysql_real_query_result(struct mysql_config *config, const char *query, MYSQL_RES **result)
{
	int ret = 0, my_err = 0, retry = 2;

	do {
		ret = mysql_real_query(config->mysql, query, strlen(query));
		if(ret < 0) {
			mlog("mysql_query: %s\n", mysql_error(config->mysql));
		}

		*result = mysql_store_result(config->mysql);
		my_err = -mysql_errno(config->mysql);
		if(*result == NULL) {
			mlog("mysql_store_result: %s %d\n", mysql_error(config->mysql), my_err);
			mysql_reset_connection(config->mysql);
		}
		printf("%s\n", __func__);
	} while(!(*result) && retry--);

	return my_err;
}

int mysql_real_query_affected(struct mysql_config *config, const char *query)
{
	int my_err = 0, retry = 2;
	int rows = 0;

	do {
		my_err = mysql_real_query(config->mysql, query, strlen(query));
		if(my_err < 0) {
			mlog("mysql_query: %s\n", mysql_error(config->mysql));
		}

		rows = mysql_affected_rows(config->mysql);
		my_err = -mysql_errno(config->mysql);
		if(my_err < 0) {
			mlog("mysql_store_result: %s %d\n", mysql_error(config->mysql), my_err);
			mysql_reset_connection(config->mysql);
		}
		printf("%s\n", __func__);
	} while(my_err < 0 && retry--);

	return (my_err < 0) ? my_err : rows;
}


int init_mysql(struct server *sv)
{
	int ret = 0;
	struct mysql_config *config = sv->mysql_config;

	config->mysql = mysql_init(NULL);

	mysql_real_connect(config->mysql, config->serverip, config->username,
		config->password, config->database_name, 0, NULL, 0);
	if(-mysql_errno(config->mysql) < 0) {
		mlog("mysql_real_connect: %s\n", mysql_error(config->mysql));
		return -mysql_errno(config->mysql);
	}
	return mysql_test_connection(sv);
}

int exit_mysql(struct server *sv)
{
	struct mysql_config *config = sv->mysql_config;
	mysql_close(config->mysql);
}

