#pragma once

#include "renderer_types.inl"

struct static_mesh_data;
struct platform_state;

b8 initialize_renderer(u64* memory_requirement, void* state);
void shutdown_renderer();

b8 renderer_initialize(const char* application_name, struct platform_state* platform_state);
void renderer_shutdown();

void renderer_set_view(mat4 view);
void renderer_on_resize(u16 width, u16 height);

b8 renderer_draw_frame(render_packet* packet);

void renderer_create_texture(
    const u8* pixels,
    struct texture* texture);
void renderer_destroy_texture(struct texture* texture);

b8 renderer_create_material(struct material* material);
void renderer_destroy_material(struct material* material);

b8 renderer_create_geometry(
    geometry* geometry,
    u32 vertex_count,
    const vertex_3d* vertices,
    u32 index_count,
    const u32* indices);
void renderer_destroy_geometry(geometry* geometry);