#pragma once

#include "define.h"

typedef struct clock {
    f64 start_time;
    f64 elasped_time;
} clock;

void clock_update(clock* c);
void clock_start(clock* c);
void clock_stop(clock* c);
