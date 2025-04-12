#include "renderer_frontend.h"

#include "renderer_backend.h"

#include "core/logger.h"
#include "core/cmemory.h"

typedef struct renderer_system_state {
    b8 initialized;
} renderer_system_state;

static renderer_system_state* state_ptr;
static renderer_backend* backend = 0;

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
    backend = callocate(sizeof(renderer_backend), MEMORY_TAG_RENDERER);
    backend->frame_number = 0;

    renderer_backend_create(RENDERER_BACKEND_VULKAN, platform_state, backend);

    if (!backend->initialize(backend, application_name, platform_state)) {
        LOG_FATAL("Failed to initialize renderer backend. Shutting down...");
        return false;
    }

    return true;
}

void renderer_shutdown() {
    if (backend) {
        backend->shutdown(backend);
        renderer_backend_destroy(backend);
        cfree(backend, sizeof(renderer_backend), MEMORY_TAG_RENDERER);
        backend = 0;
    }
}

void renderer_on_resize(u16 width, u16 height) {
    if (backend) {
        backend->resized(backend, width, height);
    } else {
        LOG_WARN("Renderer backend not initialized. Skipping resize...");
    }
}

b8 renderer_begin_frame(f32 delta_time) {
    return backend->begin_frame(backend, delta_time);
}

b8 renderer_end_frame(f32 delta_time) {
    b8 result = backend->end_frame(backend, delta_time);
    backend->frame_number++;
    return result;
}

b8 renderer_draw_frame(render_packet* packet) {
    //LOG_TRACE("Drawing frame %d", backend->frame_number);
    if (renderer_begin_frame(packet->delta_time)) {
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
