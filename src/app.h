#pragma once
#include "core/application_types.h"

struct app {
    application_configuration config;
    b8 (*initialize)(struct app* application_inst);
    b8 (*update)(struct app* application_inst, f32 delta_time);
    void (*shutdown)(struct app* application_inst);
    void (*on_resize)(struct app* application_inst, i16 width, i16 height);
    void* state;

    // application state
    void* application_state;
};