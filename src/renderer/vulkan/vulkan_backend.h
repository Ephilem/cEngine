#pragma once

#include "define.h"
#include "renderer/renderer_backend.h"
#include "resources/resource_types.h"

b8 vulkan_backend_initialize(struct renderer_backend* backend, const char* application_name, struct platform_state* platform_state);
void vulkan_backend_shutdown(struct renderer_backend* backend);

void vulkan_backend_resized(struct renderer_backend* backend, u16 width, u16 height);

b8 vulkan_backend_begin_frame(struct renderer_backend* backend, f32 delta_time);
void vulkan_backend_update_global_state(mat4 projection, mat4 view, vec3 view_position, vec4 ambient_colour, i32 mode);
b8 vulkan_backend_end_frame(struct renderer_backend* backend, f32 delta_time);

void vulkan_backend_draw_geometry(geometry_render_data data);

void vulkan_backend_create_texture(
    const u8* pixels,
    struct texture* texture);
void vulkan_backend_destroy_texture(texture * texture);

b8 vulkan_backend_create_material(struct material* material);
void vulkan_backend_destroy_material(struct material* material);

b8 vulkan_backend_create_geometry(geometry* geometry, u32 vertex_count, const vertex_3d* vertices, u32 index_count,
                                  const u32* indices);
void vulkan_backend_destroy_geometry(geometry* geometry);