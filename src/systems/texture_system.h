#pragma once

#include "renderer/renderer_types.inl"

typedef struct texture_system_config {
    u32 max_texture_count;
} texture_system_config;

#define DEFAULT_TEXTURE_NAME "default"

b8 texture_system_initialize(u64* memory_requirement, void* state, texture_system_config* config);
void texture_system_shutdown(void);

/**
 * Acquire a texture from the texture system.
 * @param name name of the texture
 * @param auto_release reference counter increase by 1. If reaching 0, the texture is released
 * @return
 */
texture* texture_system_acquire(const char* name, b8 auto_release);
void texture_system_release(const char* name);

texture* texture_system_get_default_texture();


