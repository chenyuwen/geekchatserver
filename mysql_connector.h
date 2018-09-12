#ifndef __MYSQL_CONNECTOR_H__
#define __MYSQL_CONNECTOR_H__
#include <mysql/mysql.h>
struct server;

struct mysql_config {
	MYSQL *mysql;
	unsigned char *serverip;
	unsigned char *username;
	unsigned char *password;
	unsigned char *database_name;
};

int mysql_test_connection(struct server *sv);
int init_mysql(struct server *sv);
#endif /*__MYSQL_CONNECTOR_H__*/

