#include "renderer_backend.h"

#include "core/logger.h"
#include "vulkan/vulkan_backend.h"

b8 renderer_backend_create(renderer_backend_type type, struct platform_state* platform_state, renderer_backend* out_backend) {
    out_backend->platform_state = platform_state;

    switch (type) {
        case RENDERER_BACKEND_VULKAN: {
            LOG_INFO("Renderer backend: Vulkan");
            out_backend->initialize = vulkan_backend_initialize;
            out_backend->shutdown = vulkan_backend_shutdown;
            out_backend->begin_frame = vulkan_backend_begin_frame;
            out_backend->update_global_state = vulkan_backend_update_global_state;
            out_backend->end_frame = vulkan_backend_end_frame;
            out_backend->resized = vulkan_backend_resized;
            out_backend->draw_geometry = vulkan_backend_draw_geometry;

            out_backend->create_texture = vulkan_backend_create_texture;
            out_backend->destroy_texture = vulkan_backend_destroy_texture;

            out_backend->create_material = vulkan_backend_create_material;
            out_backend->destroy_material = vulkan_backend_destroy_material;

            out_backend->create_geometry = vulkan_backend_create_geometry;
            out_backend->destroy_geometry = vulkan_backend_destroy_geometry;
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
    backend->update_global_state = 0;
    backend->end_frame = 0;
    backend->resized = 0;
    backend->draw_geometry = 0;
    backend->create_texture = 0;
    backend->destroy_texture = 0;
    backend->create_material = 0;
    backend->destroy_material = 0;
    backend->create_geometry = 0;
    backend->destroy_geometry = 0;
}
