#ifndef FPM_ENV_H
#define FPM_ENV_H 1

#include "fpm_worker_pool.h"

int fpm_env_init_child(struct fpm_worker_pool_s *wp);
int fpm_env_init_main();

extern char **environ;

#endif

