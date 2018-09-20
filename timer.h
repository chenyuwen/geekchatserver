#ifndef __TIMER_H__
#define __TIMER_H__

#include "list/gnu_list.h"

struct cbtimer;

struct cbtimer {
	struct list_head list;
	int (*handler)(struct cbtimer *, void *);
	time_t timeout;
	time_t delay_sec;
	void *arg;
};

int init_timer(struct server *sv);
int del_timer(struct server *sv, struct cbtimer *timer);
int kick_timer(struct server *sv, struct cbtimer *timer);
int add_timer(struct server *sv, struct cbtimer *timer, time_t delay_sec);
int mod_timer(struct server *sv, struct cbtimer *timer, time_t delay_sec);
int handle_timer_list(struct server *sv);
int is_timer_effective(struct server *sv, struct cbtimer *timer);

#endif /*__TIMER_H__*/

