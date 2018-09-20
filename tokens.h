#ifndef __TOKENS_H__
#define __TOKENS_H__

int init_token(struct server *sv);
int alloc_new_token(struct server *sv, struct user *usr);
int free_token(struct server *sv, struct user *usr);
int get_usr_by_token(struct server *sv, const char *token, struct user **usr);
int is_token_in_memory(struct server *sv, struct user *usr);
int is_token_effective(struct server *sv, struct user *usr);
#endif /*__TOKENS_H__*/

