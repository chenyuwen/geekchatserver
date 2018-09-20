#include <time.h>
#include "server.h"
#include "list/gnu_list.h"
#include "timer.h"

time_t get_now_time(void)
{
	time_t tm = time(NULL);
	return tm;
}

int init_timer(struct server *sv)
{
	INIT_LIST_HEAD(&sv->timers_list);
}

static inline int __put_to_timer_list(struct server *sv, struct cbtimer *timer)
{
	struct list_head *pos;
	struct cbtimer *tmp;

	list_for_each_prev(pos, &sv->timers_list) {
		tmp = list_entry(pos, struct cbtimer, list);

		if(timer->timeout > tmp->timeout) {
			break;
		}
	}
	list_add_tail(&sv->timers_list, &timer->list);
	return 0;
}

int del_timer(struct server *sv, struct cbtimer *timer)
{
	list_del(&timer->list);
	return 0;
}

int mod_timer(struct server *sv, struct cbtimer *timer, time_t delay_sec)
{
	list_del(&timer->list);
	timer->delay_sec = delay_sec;
	timer->timeout = get_now_time() + delay_sec;
	return __put_to_timer_list(sv, timer);
}

int kick_timer(struct server *sv, struct cbtimer *timer)
{
	list_del(&timer->list);
	timer->timeout = get_now_time() + timer->delay_sec;
	return __put_to_timer_list(sv, timer);
}

int add_timer(struct server *sv, struct cbtimer *timer, time_t delay_sec)
{
	timer->delay_sec = delay_sec;
	timer->timeout = get_now_time() + delay_sec;
	INIT_LIST_HEAD(&timer->list);
	return __put_to_timer_list(sv, timer);
}

int handle_timer_list(struct server *sv)
{
	time_t now = get_now_time();
	struct list_head *pos, *next;
	struct cbtimer *tmp;

	list_for_each_safe(pos, next, &sv->timers_list) {
		tmp = list_entry(pos, struct cbtimer, list);
		if(now >= tmp->timeout) {
			tmp->handler(tmp, tmp->arg);
			list_del(pos);
		} else {
			break;
		}
	}
	return 0;
}

