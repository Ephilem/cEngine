#include "core/application.h"

#include "core/event.h"
#include "app.h"
#include "clock.h"
#include "cmemory.h"
#include "logger.h"
#include "platform/platform.h"
#include "core/input.h"

#include "renderer/renderer_frontend.h"

enum application_state_enum {
    APPLICATION_STATE_STARTING = 0,
    APPLICATION_STATE_RUNNING = 1,
    APPLICATION_STATE_SUSPENDED = 2,
    APPLICATION_STATE_SHUTDOWN = 3,
};

typedef struct application_state {
    app* app_inst;
    u8 state;
    platform_state platform_state;

    i16 width;
    i16 height;
    clock clock;
    f64 last_frame_time;
} application_state;

static b8 initialized = FALSE;
static application_state app_state;

b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context);
b8 application_on_key(u16 code, void* sender, void* listener_inst, event_context context);
b8 application_on_window_resize(u16 code, void* sender, void* listener_inst, event_context context);

b8 application_create(app* app_inst) {
    if (initialized) {
        LOG_ERROR("Application state already initialized. Do not call application_create more than once.");
        return FALSE;
    }

    app_state.app_inst = app_inst;

    app_state.state = APPLICATION_STATE_STARTING;

    // Initialize subsystems
    initialize_logging();
    initialize_input();

    if (!initialize_event()) {
        LOG_FATAL("Failed to initialize event system! Shutting down.");
        return FALSE;
    }

    event_register(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_register(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_register(EVENT_CODE_KEY_RELEASED, 0, application_on_key);
    event_register(EVENT_CODE_WINDOW_RESIZE, 0, application_on_window_resize);


    if (!platform_startup(&app_state.platform_state,
        app_inst->config.window_title,
        app_inst->config.start_pos_x,
        app_inst->config.start_pos_y,
        app_inst->config.window_width,
        app_inst->config.window_height)) {
        LOG_ERROR("Platform failed to start!");
        return FALSE;
    }

    if (!renderer_initialize("cEngine", &app_state.platform_state)) {
        LOG_FATAL("Failed to initialize renderer! Shutting down.");
        return FALSE;
    }

    if (!app_state.app_inst->initialize(app_inst)) {
        LOG_FATAL("Application failed to initialize! Shutting down.");
        return FALSE;
    }

    app_state.app_inst->on_resize(app_state.app_inst, app_state.height, app_state.width);

    LOG_INFO("Platform started successfully");

    app_state.state = APPLICATION_STATE_RUNNING;

    return TRUE;
}

b8 application_run() {
    LOG_DEBUG(get_memory_usage_str());
    clock_start(&app_state.clock);
    clock_update(&app_state.clock);
    app_state.last_frame_time = app_state.clock.elasped_time;

    f64 running_time = 0.0;
    u8 frame_count = 0;
    f64 target_frame_seconds = 1.0f / 60;

    // Game loop
    while (app_state.state == APPLICATION_STATE_RUNNING) {
        if (!platform_pump_messages(&app_state.platform_state)) {
            app_state.state = APPLICATION_STATE_SHUTDOWN;
        }

        if (app_state.state != APPLICATION_STATE_SUSPENDED) {
            clock_update(&app_state.clock);
            f64 current_time = app_state.clock.elasped_time;
            f64 delta = (current_time - app_state.last_frame_time);
            f64 frame_start_time = platform_get_absolute_time();

            // Update application
            if (!app_state.app_inst->update(app_state.app_inst, (f32)delta)) {
                LOG_FATAL("Application failed to update, shutting down");
                app_state.state = APPLICATION_STATE_SHUTDOWN;
                break;
            }

            render_packet packet;
            packet.delta_time = (f32)delta;
            renderer_draw_frame(&packet);

            f64 frame_end_time = platform_get_absolute_time();
            f64 frame_elapsed_time = frame_end_time - frame_start_time;
            running_time += frame_elapsed_time;
            f64 remaining_time = target_frame_seconds - frame_elapsed_time;

            if (remaining_time > 0) {
                u64 remaining_ms = (u64)(remaining_time * 1000);

                b8 limit_frame = FALSE;
                if (remaining_ms > 0 && limit_frame) {
                    platform_sleep_ms(remaining_ms - 1);
                }

                frame_count++;
            }


            update_input(delta);

            app_state.last_frame_time = current_time;
        }
    }

    app_state.state = APPLICATION_STATE_SHUTDOWN;

    event_shutdown();
    shutdown_input();
    renderer_shutdown();
    platform_shutdown(&app_state.platform_state);

    return TRUE;
}

void application_get_framebuffer_size(u32 *width, u32 *height) {
    *width = app_state.width;
    *height = app_state.height;
}

b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context) {
    switch (code) {
        case EVENT_CODE_APPLICATION_QUIT: {
            LOG_INFO("EVENT_CODE_APPLICATION_QUIT recieved, shutting down.\n");
            app_state.state = APPLICATION_STATE_SHUTDOWN;
            return TRUE;
        }
    }

    return FALSE;
}

b8 application_on_key(u16 code, void* sender, void* listener_inst, event_context context) {
    if (code == EVENT_CODE_KEY_PRESSED) {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_ESCAPE) {
            // NOTE: Technically firing an event to itself, but there may be other listeners.
            event_context data = {};
            event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);

            // Block anything else from processing this.
            return TRUE;
        } else if (key_code == KEY_A) {
            // Example on checking for a key
            LOG_DEBUG("Explicit - A key pressed!");
        } else {
            LOG_DEBUG("'%c' key pressed in window.", key_code);
        }
    } else if (code == EVENT_CODE_KEY_RELEASED) {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_B) {
            // Example on checking for a key
            LOG_DEBUG("Explicit - B key released!");
        } else {
            LOG_DEBUG("'%c' key released in window.", key_code);
        }
    }
    return FALSE;
}

b8 application_on_window_resize(u16 code, void *sender, void *listener_inst, event_context context) {
    if (code == EVENT_CODE_WINDOW_RESIZE) {
        u16 width = context.data.u16[0];
        u16 height = context.data.u16[1];

        if (width != app_state.width || height != app_state.height) {
            app_state.width = width;
            app_state.height = height;

            if (width == 0 || height == 0) {
                LOG_INFO("Window is minimized, suspending application");
                app_state.state = APPLICATION_STATE_SUSPENDED;
            } else {
                if (app_state.state == APPLICATION_STATE_SUSPENDED) {
                    LOG_INFO("Window is restored, resuming application");
                    app_state.state = APPLICATION_STATE_RUNNING;
                }
                app_state.app_inst->on_resize(app_state.app_inst, width, height);
                renderer_on_resize(width, height);
            }
        }
    }
}
