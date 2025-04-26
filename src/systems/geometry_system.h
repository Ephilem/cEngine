#pragma once

#include "renderer/renderer_types.inl"

typedef struct geometry_system_config {
    // Max number of geoemtries that can be loaded at once
    // Should be significantyl greater than the number of stattic meshes because the
    // there can and will be more than one of theses per mesh
    u32 max_geometry_count;
} geometry_system_config;

typedef struct geometry_config {
    u32 vertex_count;
    vertex_3d* vertices;

    u32 index_count;
    u32* indices;

    char name[GEOMETRY_MAXIMUM_NAME_LENGTH];
    char material_name[MATERIAL_NAME_MAX_LENGTH];
} geometry_config;

#define DEFAULT_GEOMETRY_NAME "default"

b8 geometry_system_initialize(u64* memory_requirement, void* state, geometry_system_config config);
void geometry_system_shutdown(void* state);

/**
 * Aquires an existing geometry by id
 *
 * @param id id of the geometry to acquire
 * @return pointer to the geometry if it exists, 0 otherwise
 *
 */
geometry* geometry_system_acquire_by_id(u32 id);

/**
 * Register and acquires a new geometry using the given config
 *
 * @param config the geometry configuration
 * @param auto_release if true, the geometry will be automatically released when the reference count reaches 0
 * @return pointer to the geometry if it was successfully acquired, 0 otherwise
 */
geometry* geometry_system_acquire_from_config(geometry_config config, b8 auto_release);

void geometry_system_release(geometry* geometry);

geometry* geometry_system_get_default_geometry();

/**
 * Generate a plane geometry configuration
 * NOTE: vertex and index data are dynamically allocated and should be freed by the caller
 *
 * @param width width of the plane
 * @param height height of the plane
 * @param x_segments The number of segments in the x direction
 * @param y_segments The number of segments in the y direction
 * @param tile_x The number of time the texture is tiled in the x direction (UV config)
 * @param tile_y The number of time the texture is tiled in the y direction (UV config)
 * @param name the name of the geometry
 * @param material_name the name of the material to use
 * @return geometry_config the generated geometry configuration
 */
geometry_config geometry_system_generate_plane_config(
    f32 width, f32 height,
    u32 x_segments, u32 y_segments,
    f32 tile_x, f32 tile_y,
    const char* name,
    const char* material_name);