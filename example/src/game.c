#include "game.h"

#include <core/logger.h>
#include <core/input.h>
#include <core/cmemory.h>

static u64 alloc_count = 0;

b8 game_initialize(app* application_inst) {
    LOG_INFO("Game initialized");
    return true;
}

b8 game_update(app* application_inst, f32 delta_time) {

    u64 prev_alloc_count = alloc_count;
    alloc_count = get_memory_alloc_count();
    if (input_is_key_down('M') && input_was_key_up('M')) {
        LOG_DEBUG("Allocations: %llu (%llu this frame)", alloc_count, alloc_count - prev_alloc_count);
    }

    return true;
}

void game_shutdown(app* application_inst) {
    LOG_INFO("Game shutdown");
}

void game_on_resize(app* application_inst, i16 width, i16 height) {
    LOG_INFO("Game resized");
}

