#ifndef FPM_WORKER_POOL_H
#define FPM_WORKER_POOL_H 1

#include "fpm_conf.h"
#include "fpm_arrays.h"

struct fpm_worker_pool_s;
struct fpm_child_s;
struct fpm_child_stat_s;
struct fpm_shm_s;

enum fpm_address_domain {
	FPM_AF_UNIX = 1,
	FPM_AF_INET = 2
};

struct fpm_worker_pool_s {
	struct fpm_worker_pool_s *next;
	struct fpm_worker_pool_config_s *config;
	char *user, *home;									/* for setting env USER and HOME */
	enum fpm_address_domain listen_address_domain;
	int listening_socket;
	int set_uid, set_gid;								/* config uid and gid */
	unsigned is_template:1;									/* just config template, no processes will be created */
	int socket_uid, socket_gid, socket_mode;

	/////////////////////////////////////////////////////
    struct fpm_shm_s *shm_list;
    struct fpm_array_s slots_used;
    struct fpm_array_s slots_free;
	////////////////////////////////////////////////////

	/* runtime */
	struct fpm_child_s *children;
	int running_children;
};

struct fpm_worker_pool_s *fpm_worker_pool_alloc();
int fpm_worker_pool_init_main();

extern struct fpm_worker_pool_s *fpm_worker_all_pools;

#endif
