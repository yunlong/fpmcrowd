#ifndef FPM_CHILDREN_H
#define FPM_CHILDREN_H 1

#include <sys/time.h>
#include <sys/types.h>
#include <event.h>

#include "fpm_worker_pool.h"

int fpm_children_create_initial(struct fpm_worker_pool_s *wp);
int fpm_children_free(struct fpm_child_s *child);
void fpm_children_bury();
int fpm_children_init_main();

struct fpm_child_s;

struct fpm_child_s {
	struct fpm_child_s *prev, *next;
	struct timeval started;
	struct fpm_worker_pool_s *wp;
	struct event ev_stdout, ev_stderr;
	int shm_slot_i;
//	int fd_stdout, fd_stderr;
	int fd_stdout[2];
	int fd_stderr[2];
	void (*tracer)(struct fpm_child_s *);
	struct timeval slow_logged;
	pid_t pid;

	/////////////////// by yunlong.lee
	int child_channel_fd;   		/* parent's stream pipe to/from child */	
	struct 	fpm_array_s request_list;	/* request queue */
	unsigned int conn_count;    		/* #connections handled */
  	int child_status;   /* 0 = ready */

};

#endif
