#include "fpm_config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "fpm.h"
#include "fpm_children.h"
#include "fpm_events.h"
#include "fpm_sockets.h"
#include "fpm_stdio.h"
#include "zlog.h"

//static int fd_stdout[2];
//static int fd_stderr[2];

int fpm_stdio_init_main()
{
	int fd = open("/dev/null", O_RDWR);

	if (0 > fd) {
		zlog(ZLOG_STUFF, ZLOG_SYSERROR, "open(\"/dev/null\") failed");
		return -1;
	}

	dup2(fd, STDIN_FILENO);
	dup2(fd, STDOUT_FILENO);
	close(fd);
	
	return 0;
}

int fpm_stdio_init_final()
{
	if (fpm_global_options.daemonize) {

		if (fpm_globals.error_log_fd != STDERR_FILENO) {
			/* there might be messages to stderr from libevent, we need to log them all */
			dup2(fpm_globals.error_log_fd, STDERR_FILENO);
		}

		zlog_set_level(fpm_globals.log_level);

		zlog_set_fd(fpm_globals.error_log_fd);
	}

	zlog(ZLOG_STUFF, ZLOG_NOTICE, "error_log fd is %d  log level is %d", fpm_globals.error_log_fd, fpm_globals.log_level );

	return 0;
}

int fpm_stdio_init_child(struct fpm_worker_pool_s *wp)
{
	close(fpm_globals.error_log_fd);
	fpm_globals.error_log_fd = -1;
	zlog_set_fd(-1);

	if (wp->listening_socket != STDIN_FILENO) {
		dup2(wp->listening_socket, STDIN_FILENO);
	//	zlog(ZLOG_STUFF, ZLOG_NOTICE, "redirect child %d STDIN to listening socket %d", (int) getpid(), wp->listening_socket );
	}

//	fpm_globals.listening_socket = wp->listening_socket;
//	fpm_globals.listening_socket = 0;

	return 0;
}

static void fpm_stdio_child_said(int fd, short which, void *arg)
{
	static const int max_buf_size = 1024;
	char buf[max_buf_size];
	struct fpm_child_s *child = arg;
	int is_stdout = fd == child->fd_stdout[0];
	struct event *ev = is_stdout ? &child->ev_stdout : &child->ev_stderr;
	int fifo_in = 1, fifo_out = 1;
	int is_last_message = 0;
	int in_buf = 0;
	int res;

#if 0
	zlog(ZLOG_STUFF, ZLOG_DEBUG, "child %d said %s", (int) child->pid, is_stdout ? "stdout" : "stderr");
#endif

	while (fifo_in || fifo_out) {

		if (fifo_in) {

			res = read(fd, buf + in_buf, max_buf_size - 1 - in_buf);

			if (res <= 0) { /* no data */
				fifo_in = 0;

				if (res < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
					/* just no more data ready */
				}
				else { /* error or pipe is closed */

					if (res < 0) { /* error */
						zlog(ZLOG_STUFF, ZLOG_SYSERROR, "read() failed");
					}

					fpm_event_del(ev);
					is_last_message = 1;

					if (is_stdout) {
						close(child->fd_stdout[0]);
						child->fd_stdout[0] = -1;
					}
					else {
						close(child->fd_stderr[0]);
						child->fd_stderr[0] = -1;
					}

#if 0
					if (in_buf == 0 && !fpm_globals.is_child) {
						zlog(ZLOG_STUFF, ZLOG_DEBUG, "child %d (pool %s) %s pipe is closed", (int) child->pid,
							child->wp->config->name, is_stdout ? "stdout" : "stderr");
					}
#endif
				}
			}
			else {
				in_buf += res;
			}
		}

		if (fifo_out) {
			if (in_buf == 0) {
				fifo_out = 0;
			}
			else {
				char *nl;
				int should_print = 0;
				buf[in_buf] = '\0';

				/* FIXME: there might be binary data */

				/* we should print if no more space in the buffer */
				if (in_buf == max_buf_size - 1) {
					should_print = 1;
				}

				/* we should print if no more data to come */
				if (!fifo_in) {
					should_print = 1;
				}

				nl = strchr(buf, '\n');

				if (nl || should_print) {

					if (nl) {
						*nl = '\0';
					}

					zlog(ZLOG_STUFF, ZLOG_WARNING, "child %d (pool %s) said into %s: \"%s\"%s", (int) child->pid,
						child->wp->config->name, is_stdout ? "stdout" : "stderr", buf, is_last_message ? ", pipe is closed" : "");

					if (nl) {
						int out_buf = 1 + nl - buf;
						memmove(buf, buf + out_buf, in_buf - out_buf);
						in_buf -= out_buf;
					}
					else {
						in_buf = 0;
					}
				}
			}
		}
	}

}

int fpm_stdio_prepare_pipes(struct fpm_child_s *child)
{
	if (0 == child->wp->config->catch_workers_output) { /* not required */
		return 0;
	}

	if (0 > pipe(child->fd_stdout)) {
		zlog(ZLOG_STUFF, ZLOG_SYSERROR, "pipe() failed");
		return -1;
	}

	if (0 > pipe(child->fd_stderr)) {
		zlog(ZLOG_STUFF, ZLOG_SYSERROR, "pipe() failed");
		close(child->fd_stdout[0]); close(child->fd_stdout[1]);
		return -1;
	}
	

	if (0 > fd_set_blocked(child->fd_stdout[0], 0) || 0 > fd_set_blocked(child->fd_stderr[0], 0)) {
		zlog(ZLOG_STUFF, ZLOG_SYSERROR, "fd_set_blocked() failed");
		close(child->fd_stdout[0]); close(child->fd_stdout[1]);
		close(child->fd_stderr[0]); close(child->fd_stderr[1]);
		return -1;
	}
	
//	zlog(ZLOG_STUFF, ZLOG_NOTICE, "master process %u prepare fd_stdout fd_stderr %u %u %u %u for child", 
//			getpid(), child->fd_stdout[0], child->fd_stderr[0], child->fd_stdout[1], child->fd_stderr[1] );

	return 0;
}

int fpm_stdio_parent_use_pipes(struct fpm_child_s *child)
{
	if (0 == child->wp->config->catch_workers_output) { /* not required */
		return 0;
	}

	close(child->fd_stdout[1]);
	close(child->fd_stderr[1]);

//	child->fd_stdout = child->fd_stdout[0];
//	child->fd_stderr = child->fd_stderr[0];

	fpm_event_add(child->fd_stdout[0], &child->ev_stdout, fpm_stdio_child_said, child);
	fpm_event_add(child->fd_stderr[0], &child->ev_stderr, fpm_stdio_child_said, child);

	return 0;
}

int fpm_stdio_discard_pipes(struct fpm_child_s *child)
{
	if (0 == child->wp->config->catch_workers_output) { /* not required */
		return 0;
	}

	close(child->fd_stdout[1]);
	close(child->fd_stderr[1]);

	close(child->fd_stdout[0]);
	close(child->fd_stderr[0]);

	return 0;
}

void fpm_stdio_child_use_pipes(struct fpm_child_s *child)
{
	if (child->wp->config->catch_workers_output) {
		dup2(child->fd_stdout[1], STDOUT_FILENO);
		dup2(child->fd_stderr[1], STDERR_FILENO);

	//	zlog(ZLOG_STUFF, ZLOG_NOTICE, "redirect child %u STDOUT STDERR to %d %d ", getpid(), child->fd_stdout[1], child->fd_stderr[1] );

		close(child->fd_stdout[0]); close(child->fd_stdout[1]);
		close(child->fd_stderr[0]); close(child->fd_stderr[1]);
	}
	else {

		/* stdout of parent is always /dev/null */
		dup2(STDOUT_FILENO, STDERR_FILENO);
	}

}	

int fpm_stdio_open_error_log(int reopen)
{
	int fd;

	fd = open(fpm_global_options.error_log, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);

	if (0 > fd) {
		zlog(ZLOG_STUFF, ZLOG_SYSERROR, "open(\"%s\") failed", fpm_global_options.error_log);
		return -1;
	}

	if (reopen) {
		if (fpm_global_options.daemonize) {
			dup2(fd, STDERR_FILENO);
		}

		dup2(fd, fpm_globals.error_log_fd);
		close(fd);
		fd = fpm_globals.error_log_fd; /* for FD_CLOSEXEC to work */
	}
	else {
		fpm_globals.error_log_fd = fd;
	}

	fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);

	return 0;
}

