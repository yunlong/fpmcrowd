#include "fpm_config.h"

#if HAVE_FPM_TRACE

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>

#include "fpm_trace.h"
#include "fpm_children.h"
#include "fpm_worker_pool.h"
#include "fpm_process_ctl.h"

#include "zlog.h"

#define valid_ptr(p) ((p) && 0 == ((p) & (sizeof(long) - 1)))

#if SIZEOF_LONG == 4
#define PTR_FMT "08"
#elif SIZEOF_LONG == 8
#define PTR_FMT "016"
#endif

static int fpm_php_trace_dump(struct fpm_child_s *child, FILE *slowlog)
{
	int callers_limit = 20;
	pid_t pid = child->pid;
	struct timeval tv;
	static const int buf_size = 1024;
	char buf[buf_size];
	long execute_data;
	long l;

	gettimeofday(&tv, 0);

	zlog_print_time(&tv, buf, buf_size);

	fprintf(slowlog, "\n%s pid %d (pool %s)\n", buf, (int) pid, child->wp->config->name);

	/**
	if (0 > fpm_trace_get_strz(buf, buf_size, (long) &SG(request_info).path_translated)) {
		return -1;
	}
	***/

	fprintf(slowlog, "script_filename = %s\n", buf);

	/*
	if (0 > fpm_trace_get_long((long) &EG(current_execute_data), &l)) {
		return -1;
	}
	***/

	execute_data = l;

	while (execute_data) {
		long function;
		uint lineno = 0;

		fprintf(slowlog, "[0x%" PTR_FMT "lx] ", execute_data);
		
		/***
		if (0 > fpm_trace_get_long(execute_data + offsetof(zend_execute_data, function_state.function), &l)) {
			return -1;
		}
		***/

		function = l;

		if (valid_ptr(function)) {
				/***
			if (0 > fpm_trace_get_strz(buf, buf_size, function + offsetof(zend_function, common.function_name))) {
				return -1;
			}

				***/

			fprintf(slowlog, "%s()", buf);
		}
		else {
			fprintf(slowlog, "???");
		}

		/***
		if (0 > fpm_trace_get_long(execute_data + offsetof(zend_execute_data, op_array), &l)) {
			return -1;
		}
		***/

		*buf = '\0';

		if (valid_ptr(l)) {
			/***
			long op_array = l;
			if (0 > fpm_trace_get_strz(buf, buf_size, op_array + offsetof(zend_op_array, filename))) {
				return -1;
			}
			***/
		}

		/***
		if (0 > fpm_trace_get_long(execute_data + offsetof(zend_execute_data, opline), &l)) {
			return -1;
		}
		***/

		if (valid_ptr(l)) {
			long opline = l;
			uint *lu = (uint *) &l;
			/***
			if (0 > fpm_trace_get_long(opline + offsetof(struct _zend_op, lineno), &l)) {
				return -1;
			}
			****/

			lineno = *lu;
		}

		fprintf(slowlog, " %s:%u\n", *buf ? buf : "unknown", lineno);

		/***
		if (0 > fpm_trace_get_long(execute_data + offsetof(zend_execute_data, prev_execute_data), &l)) {
			return -1;
		}
		***/
		execute_data = l;

		if (0 == --callers_limit) {
			break;
		}
	}

	return 0;
}

void fpm_php_trace(struct fpm_child_s *child)
{
	FILE *slowlog;

	zlog(ZLOG_STUFF, ZLOG_NOTICE, "about to trace %d", (int) child->pid);

	slowlog = fopen(child->wp->config->slowlog, "a+");

	if (!slowlog) {
		zlog(ZLOG_STUFF, ZLOG_SYSERROR, "fopen(%s) failed", child->wp->config->slowlog);
		goto done0;
	}

	if (0 > fpm_trace_ready(child->pid)) {
		goto done1;
	}

	if (0 > fpm_php_trace_dump(child, slowlog)) {
		fprintf(slowlog, "+++ dump failed\n");
	}

	if (0 > fpm_trace_close(child->pid)) {
		goto done1;
	}

done1:
	fclose(slowlog);

done0:
	fpm_pctl_kill(child->pid, FPM_PCTL_CONT);
	child->tracer = 0;

	zlog(ZLOG_STUFF, ZLOG_NOTICE, "finished trace of %d", (int) child->pid);
}

int fpm_trace_get_strz(char *buf, size_t sz, long addr)
{
	int i;
	long l;
	char *lc = (char *) &l;

	if (0 > fpm_trace_get_long(addr, &l)) {
		return -1;
	}

	i = l % SIZEOF_LONG;

	l -= i;

	for (addr = l; ; addr += SIZEOF_LONG) {

		if (0 > fpm_trace_get_long(addr, &l)) {
			return -1;
		}

		for ( ; i < SIZEOF_LONG; i++) {
			--sz;

			if (sz && lc[i]) {
				*buf++ = lc[i];
				continue;
			}

			*buf = '\0';
			return 0;
		}

		i = 0;
	}
}

#endif

