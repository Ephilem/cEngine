#pragma once

#include "math/math_types.h"

#define TEXTURE_NAME_MAX_LENGTH 512

typedef struct texture {
    u32 id;
    u32 generation;

    u32 width;
    u32 height;
    u8 channel_count;
    b8 has_transparency;

    char name[TEXTURE_NAME_MAX_LENGTH];

    void* internal_data;
} texture;

typedef enum texture_use {
    TEXTURE_USE_UNKNOWN     = 0x00,
    TEXTURE_USE_MAP_DIFFUSE = 0x01,
} texture_use;

typedef struct texture_map {
    texture* texture;
    texture_use use;
} texture_map;

#define MATERIAL_NAME_MAX_LENGTH 256
typedef struct material {
    u32 id;
    u32 generation;

    u32 internal_id; // id handle to the internal material data (backend renderer representation of the material)

    char name[MATERIAL_NAME_MAX_LENGTH];

    vec4 diffuse_color;
    texture_map diffuse_map;
} material;

#define GEOMETRY_MAXIMUM_NAME_LENGTH 256

typedef struct geometry {
    u32 id;
    u32 generation;
    u32 internal_id; // id handle to the internal geometry data (backend renderer representation of the geometry)

    char name[GEOMETRY_MAXIMUM_NAME_LENGTH];

    material* material;
} geometry;

