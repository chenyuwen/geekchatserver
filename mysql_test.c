#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <mysql/mysql.h>

int main(int argc, char **argv)
{
	MYSQL *mysql = NULL;
	MYSQL_RES *result = NULL;
	MYSQL_ROW *row = NULL;
	int ret = 0;

	mysql = mysql_init(NULL);

	mysql_real_connect(mysql, "127.0.0.1", "root", "", "geekchat", 0, NULL, 0);
	if(mysql_errno(mysql) < 0) {
		printf("Connect failed.\n");
		goto out;
	}

	ret = mysql_query(mysql, "show databases");
	if(ret < 0) {
		printf("error\n");
		goto out;
	}

	result = mysql_use_result(mysql);
	if(result == NULL) {
		printf("null\n");
		goto out;
	}
	while((row = mysql_fetch_row(result)) != NULL) {
		printf("%s\n", row[0]);
	}

	mysql_free_result(result);
out:
	mysql_close(mysql);
	return 0;
}

