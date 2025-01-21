#pragma once

#include <define.h>
#include <app.h>

typedef struct game_state {
    f32 delta_time;
} game_state;

b8 game_initialize(app* application_inst);

b8 game_update(app* application_inst, f32 delta_time);

void game_shutdown(app* application_inst);

void game_on_resize(app* application_inst, i16 width, i16 height);

