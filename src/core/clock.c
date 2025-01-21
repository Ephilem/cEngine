#include "clock.h"

#include "platform/platform.h"

void clock_update(clock* c) {
    if (c->start_time != 0) {
        c->elasped_time = platform_get_absolute_time() - c->start_time;
    }
}

void clock_start(clock* c) {
    c->start_time = platform_get_absolute_time();
    c->elasped_time = 0;
}

void clock_stop(clock* c) {
    c->start_time = 0;
}

