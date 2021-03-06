#include "fpm_config.h"

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#include "fpm.h"
#include "fpm_clock.h"
#include "fpm_children.h"
#include "fpm_signals.h"
#include "fpm_events.h"
#include "fpm_process_ctl.h"
#include "fpm_cleanup.h"
#include "fpm_worker_pool.h"
#include "zlog.h"


static int fpm_state = FPM_PCTL_STATE_NORMAL;
static int fpm_signal_sent = 0;

static const char *fpm_state_names[] = {
	[FPM_PCTL_STATE_NORMAL] = "normal",
	[FPM_PCTL_STATE_RELOADING] = "reloading",
	[FPM_PCTL_STATE_TERMINATING] = "terminating",
	[FPM_PCTL_STATE_FINISHING] = "finishing"
};

static int saved_argc;
static char **saved_argv;

static void fpm_pctl_cleanup(int which, void *arg)
{
	int i;

	if (which != FPM_CLEANUP_PARENT_EXEC) {

		for (i = 0; i < saved_argc; i++) {
			free(saved_argv[i]);
		}

		free(saved_argv);

	}
}

static struct event pctl_event;

static void fpm_pctl_action(int fd, short which, void *arg)
{
	evtimer_del(&pctl_event);

	memset(&pctl_event, 0, sizeof(pctl_event));

	fpm_pctl(FPM_PCTL_STATE_UNSPECIFIED, FPM_PCTL_ACTION_TIMEOUT);
}

static int fpm_pctl_timeout_set(int sec)
{
	struct timeval tv = { .tv_sec = sec, .tv_usec = 0 };

	if (evtimer_initialized(&pctl_event)) {
		evtimer_del(&pctl_event);
	}

	evtimer_set(&pctl_event, &fpm_pctl_action, 0);

	evtimer_add(&pctl_event, &tv);

	return 0;
}

static void fpm_pctl_exit()
{
	zlog(ZLOG_STUFF, ZLOG_NOTICE, "exiting, bye-bye!");

	fpm_conf_unlink_pid();

	fpm_cleanups_run(FPM_CLEANUP_PARENT_EXIT_MAIN);

	exit(0);
}

#define optional_arg(c) (saved_argc > c ? ", \"" : ""), (saved_argc > c ? saved_argv[c] : ""), (saved_argc > c ? "\"" : "")

static void fpm_pctl_exec()
{

	zlog(ZLOG_STUFF, ZLOG_NOTICE, "reloading: execvp(\"%s\", {\"%s\""
			"%s%s%s" "%s%s%s" "%s%s%s" "%s%s%s" "%s%s%s"
			"%s%s%s" "%s%s%s" "%s%s%s" "%s%s%s" "%s%s%s"
		"})",
		saved_argv[0], saved_argv[0],
		optional_arg(1),
		optional_arg(2),
		optional_arg(3),
		optional_arg(4),
		optional_arg(5),
		optional_arg(6),
		optional_arg(7),
		optional_arg(8),
		optional_arg(9),
		optional_arg(10)
	);

	fpm_cleanups_run(FPM_CLEANUP_PARENT_EXEC);

	execvp(saved_argv[0], saved_argv);

	zlog(ZLOG_STUFF, ZLOG_SYSERROR, "execvp() failed");

	exit(1);
}

static void fpm_pctl_action_last()
{
	switch (fpm_state) {

		case FPM_PCTL_STATE_RELOADING :

			zlog(ZLOG_STUFF, ZLOG_NOTICE, "master process controller is reloading process from state %s ", fpm_state_names[fpm_state] );
			fpm_pctl_exec();
			break;

		case FPM_PCTL_STATE_FINISHING :

		case FPM_PCTL_STATE_TERMINATING :

			zlog(ZLOG_STUFF, ZLOG_NOTICE, "master process controller is exited from state %s ", fpm_state_names[fpm_state] );
			fpm_pctl_exit();
			break;
	}
}

int fpm_pctl_kill(pid_t pid, int how)
{
	int s = 0;

	switch (how) {
		case FPM_PCTL_TERM :
			s = SIGTERM;
			break;
		case FPM_PCTL_STOP :
			s = SIGSTOP;
			break;
		case FPM_PCTL_CONT :
			s = SIGCONT;
			break;
		default :
			break;
	}

	return kill(pid, s);
}

static void fpm_pctl_kill_all(int signo)
{
	struct fpm_worker_pool_s *wp;
	int alive_children = 0;

	for (wp = fpm_worker_all_pools; wp; wp = wp->next) {
		struct fpm_child_s *child;

		for (child = wp->children; child; child = child->next) {

			int res = kill(child->pid, signo);

			zlog(ZLOG_STUFF, ZLOG_NOTICE, "sending signal %d %s to child %d (pool %s)", signo,
				fpm_signal_names[signo] ? fpm_signal_names[signo] : "",
				(int) child->pid, child->wp->config->name);

			if (res == 0) ++alive_children;
		}
	}

	if (alive_children) {
		zlog(ZLOG_STUFF, ZLOG_NOTICE, "%d %s still alive", alive_children, alive_children == 1 ? "child is" : "children are");
	}
}

static void fpm_pctl_action_next()
{
	int sig, timeout;

	if (!fpm_globals.running_children) fpm_pctl_action_last();

	if (fpm_signal_sent == 0) {
		if (fpm_state == FPM_PCTL_STATE_TERMINATING) {
			sig = SIGTERM;
		}
		else {
			sig = SIGQUIT;
		}
		timeout = fpm_global_options.process_control_timeout;
	}
	else {
		if (fpm_signal_sent == SIGQUIT) {
			sig = SIGTERM;
		}
		else {
			sig = SIGKILL;
		}
		timeout = 1;
	}

	fpm_pctl_kill_all(sig);

	fpm_signal_sent = sig;

	fpm_pctl_timeout_set(timeout);
}

void fpm_pctl(int new_state, int action)
{
	switch (action) {

		case FPM_PCTL_ACTION_SET :

			if (fpm_state == new_state) { /* already in progress - just ignore duplicate signal */
				return;
			}

			switch (fpm_state) { /* check which states can be overridden */

				case FPM_PCTL_STATE_NORMAL :

					/* 'normal' can be overridden by any other state */
					break;

				case FPM_PCTL_STATE_RELOADING :

					/* 'reloading' can be overridden by 'finishing' */
					if (new_state == FPM_PCTL_STATE_FINISHING) break;

				case FPM_PCTL_STATE_FINISHING :

					/* 'reloading' and 'finishing' can be overridden by 'terminating' */
					if (new_state == FPM_PCTL_STATE_TERMINATING) break;

				case FPM_PCTL_STATE_TERMINATING :

					/* nothing can override 'terminating' state */
					zlog(ZLOG_STUFF, ZLOG_NOTICE, "not switching to '%s' state, because already in '%s' state",
						fpm_state_names[new_state], fpm_state_names[fpm_state]);

					return;
			}

			int fpm_oldstate = fpm_state;
			fpm_signal_sent = 0;
			fpm_state = new_state; 

			zlog(ZLOG_STUFF, ZLOG_NOTICE, "master process controller is switching to '%s' state from '%s' state", fpm_state_names[fpm_state], fpm_state_names[fpm_oldstate]);

			/* fall down */

		case FPM_PCTL_ACTION_TIMEOUT :

			zlog(ZLOG_STUFF, ZLOG_NOTICE, "master process controller is processing next action by '%s' state", fpm_state_names[fpm_state] );
			fpm_pctl_action_next();

			break;

		case FPM_PCTL_ACTION_LAST_CHILD_EXITED :
			zlog(ZLOG_STUFF, ZLOG_NOTICE, "master process controller is processing last child process exited" );
			fpm_pctl_action_last();

			break;

	}
}

int fpm_pctl_can_spawn_children()
{
	return fpm_state == FPM_PCTL_STATE_NORMAL;
}

int fpm_pctl_child_exited()
{
	if (fpm_state == FPM_PCTL_STATE_NORMAL) return 0;

	zlog(ZLOG_STUFF, ZLOG_NOTICE, "master process controller has %d running children", fpm_globals.running_children );
	
	if (!fpm_globals.running_children) {
		fpm_pctl(FPM_PCTL_STATE_UNSPECIFIED, FPM_PCTL_ACTION_LAST_CHILD_EXITED);
	}

	return 0;
}

int fpm_pctl_init_main()
{
	int i;

	saved_argc = fpm_globals.argc;

	saved_argv = malloc(sizeof(char *) * (saved_argc + 1));

	if (!saved_argv) {
		return -1;
	}

	for (i = 0; i < saved_argc; i++) {
		saved_argv[i] = strdup(fpm_globals.argv[i]);

		if (!saved_argv[i]) {
			return -1;
		}
	}

	saved_argv[i] = 0;

	fpm_cleanup_add(FPM_CLEANUP_ALL, fpm_pctl_cleanup, 0);

	return 0;
}

static void fpm_pctl_check_request_timeout(struct timeval *now)
{
	struct fpm_worker_pool_s *wp;

	for (wp = fpm_worker_all_pools; wp; wp = wp->next) {
		int terminate_timeout = wp->config->request_terminate_timeout;
		int slowlog_timeout = wp->config->request_slowlog_timeout;
		struct fpm_child_s *child;

		if (terminate_timeout || slowlog_timeout) {
			for (child = wp->children; child; child = child->next) {
			//	fpm_request_check_timed_out(child, now, terminate_timeout, slowlog_timeout);
			}
		}
	}
	
}

void fpm_pctl_heartbeat(int fd, short which, void *arg)
{
	static struct event heartbeat;
	struct timeval tv = { .tv_sec = 0, .tv_usec = 130000 };
	struct timeval now;

	if (which == EV_TIMEOUT) {
		evtimer_del(&heartbeat);
		fpm_clock_get(&now);
		fpm_pctl_check_request_timeout(&now);
	}

	evtimer_set(&heartbeat, &fpm_pctl_heartbeat, 0);

	evtimer_add(&heartbeat, &tv);
}

