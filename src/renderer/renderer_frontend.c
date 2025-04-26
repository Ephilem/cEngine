#include "renderer_frontend.h"

#include "renderer_backend.h"

#include "core/logger.h"
#include "core/cmemory.h"
#include "math/cmath.h"

#include "resources/resource_types.h"

// TODO temp
#include "core/cstring.h"
#include "core/event.h"
#include "systems/material_system.h"
#include "systems/texture_system.h"
// End temp

typedef struct renderer_system_state {
    b8 initialized;
    renderer_backend backend;

    mat4 projection;
    f32 near_clip;
    f32 far_clip;

    mat4 view;
} renderer_system_state;

static renderer_system_state* state_ptr;


b8 initialize_renderer(u64* memory_requirement, void* state) {
    *memory_requirement = sizeof(renderer_system_state);
    if (state == 0) {
        return false;
    }
    state_ptr = state;
    czero_memory(state_ptr, sizeof(renderer_system_state));
    state_ptr->initialized = true;

    return true;
}

void shutdown_renderer() {
    if (state_ptr) {
        state_ptr->initialized = false;
        state_ptr = 0;
    }
}

b8 renderer_initialize(const char* application_name, struct platform_state* platform_state) {
    if (!state_ptr || !state_ptr->initialized) {
        LOG_ERROR("Renderer system not initialized. Cannot initialize renderer.");
        return false;
    }

    LOG_INFO("Initializing renderer...");
    state_ptr->backend.frame_number = 0;

    renderer_backend_create(RENDERER_BACKEND_VULKAN, platform_state, &state_ptr->backend);

    if (!state_ptr->backend.initialize(&state_ptr->backend, application_name, platform_state)) {
        LOG_FATAL("Failed to initialize renderer backend. Shutting down...");
        return false;
    }

    state_ptr->projection = mat4_perspective(deg_to_rad(45.0f), 1280/720.0f, 0.1f, 1000.0f);
    state_ptr->near_clip = 0.1f;
    state_ptr->far_clip = 1000.0f;

    state_ptr->view = mat4_translation((vec3){0, 0, -30.0f});
    state_ptr->view = mat4_inverse(state_ptr->view);

    return true;
}

void renderer_set_view(mat4 view) {
    if (state_ptr && state_ptr->initialized) {
        state_ptr->view = view;
    } else {
        LOG_WARN("Renderer backend not initialized. Skipping view update...");
    }
}

void renderer_shutdown() {
    if (state_ptr) {
        state_ptr->backend.shutdown(&state_ptr->backend);
        renderer_backend_destroy(&state_ptr->backend);
    }
}

void renderer_on_resize(u16 width, u16 height) {
    if (state_ptr && state_ptr->initialized) {
        // change aspect ratio
        state_ptr->projection = mat4_perspective(
            deg_to_rad(45.0f),
            width / (f32)height,
            state_ptr->near_clip,
            state_ptr->far_clip);
        state_ptr->backend.resized(&state_ptr->backend, width, height);
    } else {
        LOG_WARN("Renderer backend not initialized. Skipping resize...");
    }
}

b8 renderer_begin_frame(f32 delta_time) {
    if (state_ptr && state_ptr->initialized) {
        return state_ptr->backend.begin_frame(&state_ptr->backend, delta_time);
    }
    return false;
}

b8 renderer_end_frame(f32 delta_time) {
    if (state_ptr && state_ptr->initialized) {
        b8 result = state_ptr->backend.end_frame(&state_ptr->backend, delta_time);
        state_ptr->backend.frame_number++;
        return result;
    }
    return false;
}

b8 renderer_draw_frame(render_packet* packet) {
    //LOG_TRACE("Drawing frame %d", state_ptr->backend.frame_number);
    if (renderer_begin_frame(packet->delta_time)) {
        state_ptr->backend.update_global_state(state_ptr->projection, state_ptr->view, vec3_zero(), vec4_one(), 0);

        u32 count = packet->geometry_count;
        for (u32 i = 0; i < count; ++i) {
            state_ptr->backend.draw_geometry(packet->geometries[i]);
        }

        b8 result = renderer_end_frame(packet->delta_time);
        if (!result) {
            LOG_FATAL("Failed to end frame. Shutting down...");
            return false;
        }
    } else {
        LOG_WARN("Failed to begin frame. Skipping frame...");
    }

    return true;
}

void renderer_create_texture(
    const u8* pixels,
    struct texture* texture) {
    if (state_ptr && state_ptr->initialized) {
        state_ptr->backend.create_texture(pixels, texture);
    } else {
        LOG_WARN("Renderer backend not initialized. Skipping texture creation...");
    }
}

void renderer_destroy_texture(struct texture* texture) {
    if (state_ptr && state_ptr->initialized) {
        state_ptr->backend.destroy_texture(texture);
    } else {
        LOG_WARN("Renderer backend not initialized. Skipping texture destruction...");
    }
}

b8 renderer_create_material(struct material* material) {
    return state_ptr->backend.create_material(material);
}

void renderer_destroy_material(struct material* material) {
    return state_ptr->backend.destroy_material(material);
}

b8 renderer_create_geometry(geometry* geometry, u32 vertex_count, const vertex_3d* vertices, u32 index_count, const u32* indices) {
    return state_ptr->backend.create_geometry(geometry, vertex_count, vertices, index_count, indices);
}

void renderer_destroy_geometry(geometry* geometry) {
    return state_ptr->backend.destroy_geometry(geometry);
}
