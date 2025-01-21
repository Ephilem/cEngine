#include "game.h"

#include <entry.h>
#include <platform/platform.h>

b8 create_application(app* application_inst) {
    application_inst->config.start_pos_x = 100;
    application_inst->config.start_pos_y = 100;
    application_inst->config.window_width = 800;
    application_inst->config.window_height = 600;
    application_inst->config.window_title = "testy";

    application_inst->initialize = game_initialize;
    application_inst->update = game_update;
    application_inst->shutdown = game_shutdown;
    application_inst->on_resize = game_on_resize;

    application_inst->state = callocate(sizeof(game_state), MEMORY_TAG_APPLICATION);

    return TRUE;
}
