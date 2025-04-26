// File to define the types used in the renderer
#pragma once

#include "define.h"
#include "math/math_types.h"
#include "resources/resource_types.h"

typedef enum renderer_backend_type {
    RENDERER_BACKEND_VULKAN,
    RENDERER_BACKEND_OPENGL,
    RENDERER_BACKEND_DIRECTX,
} renderer_backend_type;


// on nvidia card, uniform object need to be aligned to 256 bytes. So we need to pad the struct
typedef struct global_uniform_object {
    mat4 projection; // 64 bytes
    mat4 view; // 64 bytes

    mat4 m_reserved0; // 64 bytes
    mat4 m_reserved1; // 64 bytes
} global_uniform_object; // = 256 bytes

typedef struct material_uniform_object {
    vec4 diffuse_color; // 16 bytes

    vec4 v_reserved0; // 16 bytes
    vec4 v_reserved1; // 16 bytes
    vec4 v_reserved2; // 16 bytes
} material_uniform_object; // = 64 bytes

// pass to the renderer inform to how to render specific object
typedef struct geometry_render_data {
    u32 object_id;
    mat4 model;
    geometry* geometry;
} geometry_render_data;

// interface for a renderer backend
typedef struct renderer_backend {
    struct platform_state* platform_state;
    u64 frame_number;

    b8 (*initialize)(struct renderer_backend* backend, const char* application_name, struct platform_state* platform_state);
    void (*shutdown)(struct renderer_backend* backend);

    void (*resized)(struct renderer_backend* backend, u16 width, u16 height);

    // Setup the frame that will be rendered
    b8 (*begin_frame)(struct renderer_backend* backend, f32 delta_time);
    // Update the global state of the renderer (like projection, view, etc...)
    void (*update_global_state)(mat4 projection, mat4 view, vec3 view_position, vec4 ambient_colour, i32 mode);
    // Submit the frame to be rendered
    b8 (*end_frame)(struct renderer_backend* backend, f32 delta_time);

    void (*draw_geometry)(geometry_render_data data);

    void (*create_texture)(
        const u8* pixels,
        struct texture* texture);
    void (*destroy_texture)(struct texture* texture);

    b8 (*create_material)(struct material* material);
    void (*destroy_material)(struct material* material);

    b8 (*create_geometry)(geometry* geometry, u32 vertex_count, const vertex_3d* vertices, u32 index_count, const u32* indices);
    void (*destroy_geometry)(geometry* geometry);

} renderer_backend;


// Packet of data that is passed to the renderer to render a frame (like meshes, camera angles, environment, etc...)
typedef struct render_packet {
    f32 delta_time;

    u32 geometry_count;
    geometry_render_data* geometries;
} render_packet;

