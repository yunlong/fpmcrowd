#ifndef FPM_TRACE_H
#define FPM_TRACE_H 1

#include <unistd.h>

int fpm_trace_signal(pid_t pid);
int fpm_trace_ready(pid_t pid);
int fpm_trace_close(pid_t pid);
int fpm_trace_get_long(long addr, long *data);
int fpm_trace_get_strz(char *buf, size_t sz, long addr);

struct fpm_child_s;
void fpm_php_trace(struct fpm_child_s *);

#endif

