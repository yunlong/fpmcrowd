#include "fpm_config.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "fpm.h"
#include "fpm_children.h"
#include "fpm_signals.h"
#include "fpm_worker_pool.h"
#include "fpm_sockets.h"
#include "fpm_process_ctl.h"

#include "fpm_conf.h"
#include "fpm_cleanup.h"
#include "fpm_events.h"
#include "fpm_clock.h"
#include "fpm_stdio.h"
#include "fpm_unix.h"
#include "fpm_env.h"

#include "fpm_shm_slots.h"

#include "zlog.h"

static time_t *last_faults;
static int fault;

static int fpm_children_make(struct fpm_worker_pool_s *wp, int in_event_loop);

static void fpm_children_cleanup(int which, void *arg)
{
	free(last_faults);
}

static struct fpm_child_s *fpm_child_alloc()
{
	struct fpm_child_s *ret;

	ret = malloc(sizeof(struct fpm_child_s));

	if (!ret) return 0;

	memset(ret, 0, sizeof(*ret));

	return ret;
}

static void fpm_child_free(struct fpm_child_s *child)
{
	free(child);
}

static void fpm_child_close(struct fpm_child_s *child, int in_event_loop)
{
	if (child->fd_stdout[0] != -1) {
		if (in_event_loop) {
			fpm_event_fire(&child->ev_stdout);
		}
		if (child->fd_stdout[0] != -1) {
			close(child->fd_stdout[0]);
		}
	}

	if (child->fd_stderr[0] != -1) {
		if (in_event_loop) {
			fpm_event_fire(&child->ev_stderr);
		}
		if (child->fd_stderr[0] != -1) {
			close(child->fd_stderr[0]);
		}
	}

	fpm_child_free(child);
}

static void fpm_child_link(struct fpm_child_s *child)
{
	struct fpm_worker_pool_s *wp = child->wp;

	++wp->running_children;
	++fpm_globals.running_children;

	child->next = wp->children;
	if (child->next) child->next->prev = child;
	child->prev = 0;
	wp->children = child;
}

static void fpm_child_unlink(struct fpm_child_s *child)
{
	--child->wp->running_children;
	--fpm_globals.running_children;

	if (child->prev) child->prev->next = child->next;
	else child->wp->children = child->next;
	if (child->next) child->next->prev = child->prev;

}

static struct fpm_child_s *fpm_child_find(pid_t pid)
{
	struct fpm_worker_pool_s *wp;
	struct fpm_child_s *child = 0;

	for (wp = fpm_worker_all_pools; wp; wp = wp->next) {

		for (child = wp->children; child; child = child->next) {
			if (child->pid == pid) {
				break;
			}
		}

		if (child) break;
	}

	if (!child) {
		return 0;
	}

	return child;
}

static void fpm_child_init(struct fpm_worker_pool_s *wp)
{
	fpm_globals.max_requests = wp->config->max_requests;

	if (0 > fpm_stdio_init_child(wp) ||
		0 > fpm_unix_init_child(wp) ||
		0 > fpm_signals_init_child() ||
		0 > fpm_env_init_child(wp) ) {

		zlog(ZLOG_STUFF, ZLOG_ERROR, "child failed to initialize (pool %s)", wp->config->name);
		exit(255);
	}
}

int fpm_children_free(struct fpm_child_s *child)
{
	struct fpm_child_s *next;

	for (; child; child = next) {
		next = child->next;
		fpm_child_close(child, 0 /* in_event_loop */);
	}

	return 0;
}

void fpm_children_bury()
{
	int status;
	pid_t pid;
	struct fpm_child_s *child;

	while ( (pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
		char buf[128];
		int severity = ZLOG_NOTICE;

		child = fpm_child_find(pid);

		if (WIFEXITED(status)) {

			snprintf(buf, sizeof(buf), "with code %d", WEXITSTATUS(status));

			if (WEXITSTATUS(status) != 0) {
				severity = ZLOG_WARNING;
			}

		}
		else if (WIFSIGNALED(status)) {
			const char *signame = fpm_signal_names[WTERMSIG(status)];
			const char *have_core = WCOREDUMP(status) ? " (core dumped)" : "";

			if (signame == NULL) {
				signame = "";
			}

			snprintf(buf, sizeof(buf), "on signal %d %s%s", WTERMSIG(status), signame, have_core);

			if (WTERMSIG(status) != SIGQUIT) { /* possible request loss */
				severity = ZLOG_WARNING;
			}
		}
		else if (WIFSTOPPED(status)) {

			zlog(ZLOG_STUFF, ZLOG_NOTICE, "child %d stopped for tracing", (int) pid);

			if (child && child->tracer) {
				child->tracer(child);
			}

			continue;
		}

		if (child) {
			struct fpm_worker_pool_s *wp = child->wp;
			struct timeval tv1, tv2;

			fpm_child_unlink(child);

			fpm_shm_slots_discard_slot(child);

			fpm_clock_get(&tv1);

			timersub(&tv1, &child->started, &tv2);

			zlog(ZLOG_STUFF, severity, "child %d (pool %s) exited %s after %ld.%06d seconds from start", (int) pid,
						child->wp->config->name, buf, tv2.tv_sec, (int) tv2.tv_usec);

			fpm_child_close(child, 1 /* in event_loop */);

			fpm_pctl_child_exited();

			if (last_faults && (WTERMSIG(status) == SIGSEGV || WTERMSIG(status) == SIGBUS)) {
				time_t now = tv1.tv_sec;
				int restart_condition = 1;
				int i;

				last_faults[fault++] = now;

				if (fault == fpm_global_options.emergency_restart_threshold) {
					fault = 0;
				}

				for (i = 0; i < fpm_global_options.emergency_restart_threshold; i++) {
					if (now - last_faults[i] > fpm_global_options.emergency_restart_interval) {
						restart_condition = 0;
						break;
					}
				}

				if (restart_condition) {

					zlog(ZLOG_STUFF, ZLOG_WARNING, "failed processes threshold (%d in %d sec) is reached, initiating reload",
						fpm_global_options.emergency_restart_threshold, fpm_global_options.emergency_restart_interval);

					fpm_pctl(FPM_PCTL_STATE_RELOADING, FPM_PCTL_ACTION_SET);
				}
			}

			fpm_children_make(wp, 1 /* in event loop */);

			if (fpm_globals.is_child) {
				break;
			}
		}
		else {
			zlog(ZLOG_STUFF, ZLOG_ALERT, "oops, unknown child exited %s", buf);
		}
	}

}

static struct fpm_child_s *fpm_resources_prepare(struct fpm_worker_pool_s *wp)
{
	struct fpm_child_s *c;

	c = fpm_child_alloc();

	if (!c) {
		zlog(ZLOG_STUFF, ZLOG_ERROR, "malloc failed (pool %s)", wp->config->name);
		return 0;
	}

	c->wp = wp;
//	c->fd_stdout = -1; c->fd_stderr = -1;
	c->fd_stdout[0] = -1; c->fd_stderr[0] = -1;
	c->fd_stdout[1] = -1; c->fd_stderr[1] = -1;

	if (0 > fpm_stdio_prepare_pipes(c)) {
		fpm_stdio_discard_pipes(c);
		fpm_child_free(c);
		return 0;
	}

    if (0 > fpm_shm_slots_prepare_slot(c)) {
        fpm_stdio_discard_pipes(c);
        fpm_child_free(c);
        return 0;
    }   

	return c;
}

static void fpm_resources_discard(struct fpm_child_s *child)
{
	fpm_shm_slots_discard_slot(child);
	fpm_stdio_discard_pipes(child);
	fpm_child_free(child);
}

static void fpm_child_resources_use(struct fpm_child_s *child)
{
	fpm_shm_slots_child_use_slot(child);
	fpm_stdio_child_use_pipes(child);
	fpm_child_free(child);
}

static void fpm_parent_resources_use(struct fpm_child_s *child)
{
	fpm_shm_slots_parent_use_slot(child);
	fpm_stdio_parent_use_pipes(child);
	fpm_child_link(child);
}

static int fpm_children_make(struct fpm_worker_pool_s *wp, int in_event_loop)
{
	int enough = 0;
	pid_t pid;
	struct fpm_child_s *child;

	while (!enough && fpm_pctl_can_spawn_children() && wp->running_children < wp->config->pm->max_children) {

		child = fpm_resources_prepare(wp);

		if (!child) {
			enough = 1;
			break;
		}

		pid = fork();

		switch (pid) {

			case 0 :
			//	zlog(ZLOG_STUFF, ZLOG_NOTICE, "init child %d resource", (int) getpid() );
				fpm_child_resources_use(child);
				fpm_globals.is_child = 1;

				fpm_globals.running_children = 0;

				if (in_event_loop) {
					fpm_event_exit_loop();
				}
				fpm_child_init(wp);
				return 0;

			case -1 :
				zlog(ZLOG_STUFF, ZLOG_SYSERROR, "fork() failed");
				enough = 1;

				fpm_resources_discard(child);

				break; /* dont try any more on error */

			default :
				child->pid = pid;
				fpm_clock_get(&child->started);
				fpm_parent_resources_use(child);

				zlog(ZLOG_STUFF, ZLOG_NOTICE, "child %d (pool %s) started", (int) pid, wp->config->name);
		}

	}

	return 1; /* we are done */
}

int fpm_children_create_initial(struct fpm_worker_pool_s *wp)
{
	return fpm_children_make(wp, 0 /* not in event loop yet */);
}

int fpm_children_init_main()
{
	if (fpm_global_options.emergency_restart_threshold &&
		fpm_global_options.emergency_restart_interval) {

		last_faults = malloc(sizeof(time_t) * fpm_global_options.emergency_restart_threshold);

		if (!last_faults) {
			return -1;
		}

		memset(last_faults, 0, sizeof(time_t) * fpm_global_options.emergency_restart_threshold);
	}

	fpm_cleanup_add(FPM_CLEANUP_ALL, fpm_children_cleanup, 0);

	return 0;
}

