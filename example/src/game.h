#pragma once

#include <define.h>
#include <app.h>

#include <math/math_types.h>

typedef struct game_state {
    f32 delta_time;
    mat4 view;

    // camera
    vec3 camera_position;
    vec3 camera_euler;
    b8 camera_updated;
} game_state;

b8 game_initialize(app* application_inst);

b8 game_update(app* application_inst, f32 delta_time);

void game_shutdown(app* application_inst);

void game_on_resize(app* application_inst, i16 width, i16 height);

