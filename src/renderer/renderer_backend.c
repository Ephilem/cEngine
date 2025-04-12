#include "renderer_backend.h"

#include "core/logger.h"
#include "vulkan/vulkan_backend.h"

b8 renderer_backend_create(renderer_backend_type type, struct platform_state* platform_state, renderer_backend* out_backend) {
    out_backend->platform_state = platform_state;

    switch (type) {
        case RENDERER_BACKEND_VULKAN: {
            LOG_DEBUG("Renderer backend: Vulkan");
            out_backend->initialize = vulkan_renderer_backend_initialize;
            out_backend->shutdown = vulkan_renderer_backend_shutdown;
            out_backend->begin_frame = vulkan_renderer_backend_begin_frame;
            out_backend->end_frame = vulkan_renderer_backend_end_frame;
            out_backend->resized = vulkan_renderer_backend_resized;
        } break;
        /*case RENDERER_BACKEND_OPENGL: {
            return renderer_backend_opengl_create(platform_state, out_backend);
        } break;
        case RENDERER_BACKEND_DIRECTX: {
            return renderer_backend_directx_create(platform_state, out_backend);
        } break;*/
        default: {
            return false;
        } break;
    }

    return false;
}

void renderer_backend_destroy(renderer_backend* backend) {
    backend->initialize = 0;
    backend->shutdown = 0;
    backend->begin_frame = 0;
    backend->end_frame = 0;
    backend->resized = 0;
}
