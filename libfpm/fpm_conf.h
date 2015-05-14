#ifndef FPM_CONF_H
#define FPM_CONF_H 1

struct key_value_s;

struct key_value_s {
	struct key_value_s *next;
	char *key;
	char *value;
};

struct fpm_options_s {
	int emergency_restart_threshold;
	int emergency_restart_interval;
	int process_control_timeout;
	int daemonize;
	char *pid_file;
	char *error_log;
};

extern struct fpm_options_s fpm_global_options;

struct fpm_pm_s {
	int style;
	int max_children;
	struct {
		int StartServers;
		int MinSpareServers;
		int MaxSpareServers;
	} options_apache_like;
};

struct fpm_listen_options_s {
	int backlog;
	char *owner;
	char *group;
	char *mode;
};

struct fpm_worker_pool_config_s {
	char *name;
	char *listen_address;
	struct fpm_listen_options_s *listen_options;
	struct key_value_s *php_defines;
	char *user;
	char *group;
	char *chroot;
	char *chdir;
	char *allowed_clients;
	struct key_value_s *environment;
	struct fpm_pm_s *pm;
	int request_terminate_timeout;
	int request_slowlog_timeout;
	char *slowlog;
	int max_requests;
	int rlimit_files;
	int rlimit_core;
	unsigned catch_workers_output:1;
};

enum { PM_STYLE_STATIC = 1, PM_STYLE_APACHE_LIKE = 2 };

int fpm_conf_init_main();
int fpm_worker_pool_config_free(struct fpm_worker_pool_config_s *wpc);
int fpm_conf_write_pid();
int fpm_conf_unlink_pid();

#endif

