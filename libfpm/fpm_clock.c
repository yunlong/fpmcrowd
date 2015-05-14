#include "fpm_config.h"
#include "fpm_clock.h"
#include "zlog.h"

int fpm_clock_init()
{
	return 0;
}

int fpm_clock_get(struct timeval *tv)
{
	return gettimeofday(tv, 0);
}

