#pragma once

#include "renderer_types.inl"

struct static_mesh_data;
struct platform_state;

b8 renderer_initialize(const char* application_name, struct platform_state* platform_state);
void renderer_shutdown();

void renderer_on_resize(u16 width, u16 height);

b8 renderer_draw_frame(render_packet* packet);
