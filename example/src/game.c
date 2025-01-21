#include "game.h"

#include <core/logger.h>

b8 game_initialize(app* application_inst) {
    LOG_INFO("Game initialized");
    return TRUE;
}

b8 game_update(app* application_inst, f32 delta_time) {
    return TRUE;
}

void game_shutdown(app* application_inst) {
    LOG_INFO("Game shutdown");
}

void game_on_resize(app* application_inst, i16 width, i16 height) {
    LOG_INFO("Game resized");
}

