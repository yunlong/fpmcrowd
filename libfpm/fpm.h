#ifndef FPM_H
#define FPM_H 1

#include <unistd.h>

int fpm_run(int *max_requests);
int fpm_init(int argc, char **argv, char *config);

struct fpm_globals_s {
	pid_t parent_pid;
	int argc;
	char **argv;
	char *config;
	int running_children;
	int error_log_fd;
	int log_level;
	int listening_socket; /* for this child */
	int max_requests; /* for this child */
	int is_child;
};

extern struct fpm_globals_s fpm_globals;

extern int fpm;

#endif
