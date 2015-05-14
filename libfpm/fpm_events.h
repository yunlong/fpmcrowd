#ifndef FPM_EVENTS_H
#define FPM_EVENTS_H 1

void fpm_event_exit_loop();
int fpm_event_loop();
int fpm_event_add(int fd, struct event *ev, void (*callback)(int, short, void *), void *arg);
int fpm_event_del(struct event *ev);
void fpm_event_fire(struct event *ev);
int fpm_event_init_main();


#endif
