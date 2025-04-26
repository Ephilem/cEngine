#include "core/application.h"

#include "core/event.h"
#include "app.h"
#include "clock.h"
#include "cmemory.h"
#include "logger.h"
#include "platform/platform.h"
#include "core/input.h"
#include "memory/linear_allocator.h"
#include "cstring.h"
#include "math/cmath.h"

#include "renderer/renderer_frontend.h"
#include "systems/geometry_system.h"
#include "systems/material_system.h"
#include "systems/texture_system.h"

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
    renderer_backend renderer_backend;

    i16 width;
    i16 height;
    clock clock;
    f64 last_frame_time;

    linear_allocator systems_allocator;

    u64 event_system_memory_requirement;
    void* event_system_state;

    u64 memory_system_memory_requirement;
    void* memory_system_state;

    u64 logging_system_memory_requirement;
    void* logging_system_state;

    u64 input_system_memory_requirement;
    void* input_system_state;

    u64 platform_system_memory_requirement;
    void* platform_system_state;

    u64 renderer_system_memory_requirement;
    void* renderer_system_state;

    u64 texture_system_memory_requirement;
    void* texture_system_state;

    u64 material_system_memory_requirement;
    void* material_system_state;

    u64 geometry_system_memory_requirement;
    void* geometry_system_state;

    geometry* test_geometry;

} application_state;

static b8 initialized = false;
static application_state* app_state; // pointer to the application's instance application state

b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context);
b8 application_on_key(u16 code, void* sender, void* listener_inst, event_context context);
b8 application_on_window_resize(u16 code, void* sender, void* listener_inst, event_context context);

b8 event_on_debug_event(u16 code, void* sender, void* listener_inst, event_context data) {
    const char* names[3] = {
        "cobblestone",
        "clay",
        "bookshelf",
    };
    static i8 choice = 2;
    const char* old_name = names[choice];

    choice++;
    choice %= 3;

    if (app_state->test_geometry) {
        app_state->test_geometry->material->diffuse_map.texture = texture_system_acquire(names[choice], true);
        if (!app_state->test_geometry->material->diffuse_map.texture) {
            LOG_WARN("failed to acquire texture '%s'. using default", names[choice]);
            app_state->test_geometry->material->diffuse_map.texture = texture_system_get_default_texture();
        }

        texture_system_release(old_name);
    }

    return true;
}

b8 application_create(app* app_inst) {
    if (app_inst->application_state) {
        LOG_ERROR("Application state already initialized. Do not call application_create more than once.");
        return false;
    }

    app_inst->application_state = callocate(sizeof(application_state), MEMORY_TAG_APPLICATION);

    app_state = (application_state*)app_inst->application_state;
    app_state->app_inst = app_inst;
    app_state->state = APPLICATION_STATE_STARTING;

    u64 systems_allocator_total_size = 1024 * 1024 * 64; // 10 MB
    linear_allocator_create(systems_allocator_total_size, 0, &app_state->systems_allocator);
    
    // Vérifions si l'allocateur a été correctement initialisé
    if (!app_state->systems_allocator.memory) {
        LOG_FATAL("Failed to create systems allocator! Shutting down.");
        return false;
    }

    app_state->width = app_inst->config.window_width;

    //// Initialize subsystems
    // memory
    initialize_memory(&app_state->memory_system_memory_requirement, 0);
    app_state->memory_system_state = linear_allocator_allocate(
        &app_state->systems_allocator, app_state->memory_system_memory_requirement);
    if (!initialize_memory(&app_state->memory_system_memory_requirement, app_state->memory_system_state)) {
        LOG_FATAL("Failed to initialize memory system! Shutting down.");
        return false;
    }

    // logging
    initialize_logging(&app_state->logging_system_memory_requirement, 0);
    app_state->logging_system_state = linear_allocator_allocate(
        &app_state->systems_allocator, app_state->logging_system_memory_requirement);
    if (!initialize_logging(&app_state->logging_system_memory_requirement, app_state->logging_system_state)) {
        LOG_FATAL("Failed to initialize logging system! Shutting down.");
        return false;
    }

    // events
    initialize_event(&app_state->event_system_memory_requirement, 0);
    app_state->event_system_state = linear_allocator_allocate(
        &app_state->systems_allocator, app_state->event_system_memory_requirement);
    if (!initialize_event(&app_state->event_system_memory_requirement, app_state->event_system_state)) {
        LOG_FATAL("Failed to initialize event system! Shutting down.");
        return false;
    }

    // Input system
    initialize_input(&app_state->input_system_memory_requirement, 0);
    app_state->input_system_state = linear_allocator_allocate(
        &app_state->systems_allocator, app_state->input_system_memory_requirement);
    if (!initialize_input(&app_state->input_system_memory_requirement, app_state->input_system_state)) {
        LOG_FATAL("Failed to initialize input system! Shutting down.");
        return false;
    }

    // Platform system
    initialize_platform(&app_state->platform_system_memory_requirement, 0);
    app_state->platform_system_state = linear_allocator_allocate(
        &app_state->systems_allocator, app_state->platform_system_memory_requirement);
    if (!initialize_platform(&app_state->platform_system_memory_requirement, app_state->platform_system_state)) {
        LOG_FATAL("Failed to initialize platform system! Shutting down.");
        return false;
    }

    // Renderer system
    initialize_renderer(&app_state->renderer_system_memory_requirement, 0);
    app_state->renderer_system_state = linear_allocator_allocate(
        &app_state->systems_allocator, app_state->renderer_system_memory_requirement);
    if (!initialize_renderer(&app_state->renderer_system_memory_requirement, app_state->renderer_system_state)) {
        LOG_FATAL("Failed to initialize renderer system! Shutting down.");
        return false;
    }


    // Configurer les informations de la fenêtre
    app_state->platform_state.window_title = app_inst->config.window_title;
    app_state->platform_state.x = app_inst->config.start_pos_x;
    app_state->platform_state.y = app_inst->config.start_pos_y;
    app_state->platform_state.width = app_inst->config.window_width;
    app_state->platform_state.height = app_inst->config.window_height;

    // Créer la fenêtre
    if (!create_window(&app_state->platform_state)) {
        LOG_ERROR("Platform failed to create window!");
        return false;
    }

    if (!renderer_initialize("cEngine", &app_state->platform_state)) {
        LOG_FATAL("Failed to initialize renderer! Shutting down.");
        return false;
    }

    // Texture system
    texture_system_config texture_sys_config;
    texture_sys_config.max_texture_count = 65536;
    texture_system_initialize(&app_state->texture_system_memory_requirement, 0, &texture_sys_config);
    app_state->texture_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->texture_system_memory_requirement);
    if (!texture_system_initialize(&app_state->texture_system_memory_requirement, app_state->texture_system_state, &texture_sys_config)) {
        LOG_FATAL("Failed to initialize texture system! Shutting down.");
        return false;
    }

    // Material system
    material_system_config material_sys_config;
    material_sys_config.max_material_count = 65536;
    material_system_initialize(&app_state->material_system_memory_requirement, 0, material_sys_config);
    app_state->material_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->material_system_memory_requirement);
    if (!material_system_initialize(&app_state->material_system_memory_requirement, app_state->material_system_state, material_sys_config)) {
        LOG_FATAL("Failed to initialize material system! Shutting down.");
        return false;
    }

    // Geometry system
    geometry_system_config geometry_sys_config;
    geometry_sys_config.max_geometry_count = 4096;
    geometry_system_initialize(&app_state->geometry_system_memory_requirement, 0, geometry_sys_config);
    app_state->geometry_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->geometry_system_memory_requirement);
    if (!geometry_system_initialize(&app_state->geometry_system_memory_requirement, app_state->geometry_system_state, geometry_sys_config)) {
        LOG_FATAL("Failed to initialize geometry system! Shutting down.");
        return false;
    }

    // todo: temp

    geometry_config g_config = geometry_system_generate_plane_config(10.0f, 10.0f, 5, 5, 2.0f, 2.0f, "test_plane", "test_material");
    app_state->test_geometry = geometry_system_acquire_from_config(g_config, true);

    cfree(g_config.vertices, sizeof(vertex_3d) * g_config.vertex_count, MEMORY_TAG_ARRAY);
    cfree(g_config.indices, sizeof(u32) * g_config.index_count, MEMORY_TAG_ARRAY);

    // todo: end temp

    event_register(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_register(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_register(EVENT_CODE_KEY_RELEASED, 0, application_on_key);
    event_register(EVENT_CODE_WINDOW_RESIZE, 0, application_on_window_resize);
    event_register(EVENT_CODE_DEBUG0, 0, event_on_debug_event);

    if (!app_state->app_inst->initialize(app_inst)) {
        LOG_FATAL("Application failed to initialize! Shutting down.");
        return false;
    }

    app_state->app_inst->on_resize(app_state->app_inst, app_state->height, app_state->width);

    LOG_INFO("Platform started successfully");

    app_state->state = APPLICATION_STATE_RUNNING;

    return true;
}

b8 application_run() {
    LOG_DEBUG(get_memory_usage_str());
    clock_start(&app_state->clock);
    clock_update(&app_state->clock);
    app_state->last_frame_time = app_state->clock.elasped_time;

    f64 running_time = 0.0;
    u8 frame_count = 0;
    f64 target_frame_seconds = 1.0f / 60;

    // Game loop
    while (app_state->state == APPLICATION_STATE_RUNNING) {
        if (!platform_pump_messages(&app_state->platform_state)) {
            app_state->state = APPLICATION_STATE_SHUTDOWN;
        }

        if (app_state->state != APPLICATION_STATE_SUSPENDED) {
            clock_update(&app_state->clock);
            f64 current_time = app_state->clock.elasped_time;
            f64 delta = (current_time - app_state->last_frame_time);
            f64 frame_start_time = platform_get_absolute_time();

            // Update application
            if (!app_state->app_inst->update(app_state->app_inst, (f32)delta)) {
                LOG_FATAL("Application failed to update, shutting down");
                app_state->state = APPLICATION_STATE_SHUTDOWN;
                break;
            }

            render_packet packet;
            packet.delta_time = (f32)delta;

            // TODO: temp
            geometry_render_data test_render;
            test_render.geometry = app_state->test_geometry;
            test_render.model = mat4_identity();

            packet.geometries = &test_render;
            packet.geometry_count = 1;
            // TODO: end temp

            renderer_draw_frame(&packet);

            f64 frame_end_time = platform_get_absolute_time();
            f64 frame_elapsed_time = frame_end_time - frame_start_time;
            running_time += frame_elapsed_time;
            f64 remaining_time = target_frame_seconds - frame_elapsed_time;

            if (remaining_time > 0) {
                u64 remaining_ms = (u64)(remaining_time * 1000);

                b8 limit_frame = false;
                if (remaining_ms > 0 && limit_frame) {
                    platform_sleep_ms(remaining_ms - 1);
                }

                frame_count++;
            }


            update_input(delta);

            app_state->last_frame_time = current_time;
        }
    }

    app_state->state = APPLICATION_STATE_SHUTDOWN;

    event_unregister(EVENT_CODE_DEBUG0, 0, event_on_debug_event);
    event_unregister(EVENT_CODE_WINDOW_RESIZE, 0, application_on_window_resize);
    event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_unregister(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_unregister(EVENT_CODE_KEY_RELEASED, 0, application_on_key);

    if (app_state->event_system_state) {
        event_shutdown();
    }

    if (app_state->geometry_system_state) {
        geometry_system_shutdown(app_state->geometry_system_state);
    }

    if (app_state->material_system_state) {
        material_system_shutdown(app_state->material_system_state);
    }

    if (app_state->texture_system_state) {
        texture_system_shutdown();
    }

    // Vérifier si le système d'input a été correctement initialisé avant de le fermer
    if (app_state->input_system_state) {
        shutdown_input();
    }
    
    renderer_shutdown();
    if (app_state->renderer_system_state) {
        shutdown_renderer();
    }
    
    if (app_state->platform_system_state) {
        shutdown_platform();
    }

    return true;
}

void application_get_framebuffer_size(u32 *width, u32 *height) {
    *width = app_state->width;
    *height = app_state->height;
}

b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context) {
    switch (code) {
        case EVENT_CODE_APPLICATION_QUIT: {
            LOG_INFO("EVENT_CODE_APPLICATION_QUIT recieved, shutting down.\n");
            app_state->state = APPLICATION_STATE_SHUTDOWN;
            return true;
        }
    }

    return false;
}

b8 application_on_key(u16 code, void* sender, void* listener_inst, event_context context) {
    if (code == EVENT_CODE_KEY_PRESSED) {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_ESCAPE) {
            // NOTE: Technically firing an event to itself, but there may be other listeners.
            event_context data = {};
            event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);

            // Block anything else from processing this.
            return true;
        } else if (key_code == KEY_A) {
            // Example on checking for a key
            //LOG_DEBUG("Explicit - A key pressed!");
        } else {
            //LOG_DEBUG("'%c' key pressed in window.", key_code);
        }
    } else if (code == EVENT_CODE_KEY_RELEASED) {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_B) {
            // Example on checking for a key
            //LOG_DEBUG("Explicit - B key released!");
        } else {
            //LOG_DEBUG("'%c' key released in window.", key_code);
        }
    }
    return false;
}

b8 application_on_window_resize(u16 code, void *sender, void *listener_inst, event_context context) {
    if (code == EVENT_CODE_WINDOW_RESIZE) {
        u16 width = context.data.u16[0];
        u16 height = context.data.u16[1];

        if (width != app_state->width || height != app_state->height) {
            app_state->width = width;
            app_state->height = height;

            if (width == 0 || height == 0) {
                LOG_INFO("Window is minimized, suspending application");
                app_state->state = APPLICATION_STATE_SUSPENDED;
            } else {
                if (app_state->state == APPLICATION_STATE_SUSPENDED) {
                    LOG_INFO("Window is restored, resuming application");
                    app_state->state = APPLICATION_STATE_RUNNING;
                }
                app_state->app_inst->on_resize(app_state->app_inst, width, height);
                renderer_on_resize(width, height);
            }
        }
    }
}
