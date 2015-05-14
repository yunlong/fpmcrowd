#include "fpm_config.h"

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fpm_env.h"
#include "zlog.h"

int fpm_env_init_child(struct fpm_worker_pool_s *wp)
{
	struct key_value_s *kv;

	clearenv();

	for (kv = wp->config->environment; kv; kv = kv->next) {
		setenv(kv->key, kv->value, 1);
	}

	if (wp->user) {
		setenv("USER", wp->user, 1);
	}

	if (wp->home) {
		setenv("HOME", wp->home, 1);
	}

	return 0;
}

static int fpm_env_conf_wp(struct fpm_worker_pool_s *wp)
{
	struct key_value_s *kv;

	kv = wp->config->environment;

	for (kv = wp->config->environment; kv; kv = kv->next) {
		if (*kv->value == '$') {
			char *value = getenv(kv->value + 1);

			if (!value) value = "";

			free(kv->value);
			kv->value = strdup(value);
		}

		/* autodetected values should be removed
			if these vars specified in config */
		if (!strcmp(kv->key, "USER")) {
			free(wp->user);
			wp->user = 0;
		}

		if (!strcmp(kv->key, "HOME")) {
			free(wp->home);
			wp->home = 0;
		}
	}

	zlog(ZLOG_STUFF, ZLOG_NOTICE, "home:%s user:%s \n", wp->user,wp->home);

	return 0;
}

int fpm_env_init_main()
{
	struct fpm_worker_pool_s *wp;

	for (wp = fpm_worker_all_pools; wp; wp = wp->next) {

		if (0 > fpm_env_conf_wp(wp)) {
			return -1;
		}

	}

	return 0;
}
