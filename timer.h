#ifndef __TIMER_H
#define __TIMER_H

/* linux-centric code alert. */

#include <sys/time.h>
#include <unistd.h>

typedef struct {
    struct timeval tv_start;
    struct timeval tv_end;
} Timer;

void timer_start(Timer* t);
void timer_stop(Timer* t);
long timer_get_elapsed(Timer* t);

#endif /* __TIMER_H */
