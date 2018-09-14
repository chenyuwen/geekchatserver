#ifndef __MLOG_H__
#define __MLOG_H__

struct server;

int mlog(const char *format, ...);
int merr(const char *str);
int init_mlog(struct server *sv);

#endif /*__MLOG_H__*/

