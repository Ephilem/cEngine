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
    const char* name,
    i32 width,
    i32 height,
    i32 channel_count,
    const u8* pixels,
    b8 has_transparency,
    struct texture* out_texture);
void renderer_destroy_texture(struct texture* texture);