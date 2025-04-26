#include "texture_system.h"

#include "core/logger.h"
#include "core/cmemory.h"
#include "core/cstring.h"
#include "containers/hashtable.h"

#include "renderer/renderer_frontend.h"

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"

typedef struct texture_system_state {
    texture_system_config config;
    texture default_texture;

    texture* registered_textures;

    hashtable registered_texture_table;
} texture_system_state;

typedef struct texture_reference {
    u64 reference_count;
    u32 handle;
    b8 auto_release;
} texture_reference;

static texture_system_state* state_ptr = 0;

b8 create_default_texture(texture* texture);
void destroy_default_texture(texture_system_state* state);

b8 load_texture(const char* texture_name, texture* t);

b8 texture_system_initialize(u64 *memory_requirement, void *state, texture_system_config *config) {
    if (config->max_texture_count == 0) {
        LOG_FATAL("Can't initialize texture system with 0 max texture count");
        return false;
    }

    // block of memory will contain state structure, then block for array, then block for hashtable
    u64 struct_requirement = sizeof(texture_system_state);
    u64 array_requirement = sizeof(texture) * config->max_texture_count;
    u64 hashtable_requirement = sizeof(texture_reference) * config->max_texture_count;
    *memory_requirement = struct_requirement + array_requirement + hashtable_requirement;

    if (!state) {
        return true;
    }

    state_ptr = state;
    state_ptr->config = *config;

    void* array_block = state + struct_requirement;
    state_ptr->registered_textures = array_block;

    void* hashtable_block = array_block + array_requirement;
    hashtable_create(sizeof(texture_reference), config->max_texture_count, hashtable_block, false, &state_ptr->registered_texture_table);

    // fill the hashtable with invalid refenreces to use as a default
    texture_reference invalid_ref;
    invalid_ref.auto_release = false;
    invalid_ref.handle = INVALID_ID;
    invalid_ref.reference_count = 0;
    hashtable_fill(&state_ptr->registered_texture_table, &invalid_ref);

    u32 count = state_ptr->config.max_texture_count;
    for (u32 i = 0; i < count; ++i) {
        state_ptr->registered_textures[i].id = INVALID_ID;
        state_ptr->registered_textures[i].generation = INVALID_ID;
    }

    create_default_texture(&state_ptr->default_texture);

    LOG_INFO("Texture system initialized with %i textures", state_ptr->config.max_texture_count);

    return true;
}

void texture_system_shutdown() {
    if (state_ptr) {
        // destroy all textures
        for (u32 i = 0; i < state_ptr->config.max_texture_count; ++i) {
            texture* t = &state_ptr->registered_textures[i];
            if (t->generation != INVALID_ID) {
                renderer_destroy_texture(t);
            }
        }

        destroy_default_texture(state_ptr);

        state_ptr = 0;
    }
}

texture* texture_system_acquire(const char *name, b8 auto_release) {
    // return defualt texture, but warn about because its a misuse
    if (string_equals_case(name, DEFAULT_TEXTURE_NAME)) {
        LOG_WARN("You should use texture_system_get_default_texture() instead of texture_system_acquire() for the default texture");
        return &state_ptr->default_texture;
    }

    texture_reference ref;
    if (state_ptr && hashtable_get(&state_ptr->registered_texture_table, name, &ref)) {
        if (ref.reference_count == 0) {
            ref.auto_release = auto_release;
        }
        ref.reference_count++;
        if (ref.handle == INVALID_ID) {
            // this means no texture exists here. find a free index first
            u32 count = state_ptr->config.max_texture_count;
            texture* t;
            for (u32 i = 0; i < count; ++i) {
                if (state_ptr->registered_textures[i].id == INVALID_ID) {
                    ref.handle = i;
                    t = &state_ptr->registered_textures[i];
                    break;
                }
            }

            // make sure we found a free index
            if (!t || ref.handle == INVALID_ID) {
                LOG_FATAL("Texture system cannot hold anymore textures. adjust configuration to allow more textures");
                return 0;
            }

            if (!load_texture(name, t)) {
                LOG_ERROR("Failed to load texture '%s'", name);
                return 0;
            }

            // also use the handle as the texture id.
            t->id = ref.handle;
            LOG_TRACE("Texture '%s' does not yet exist. Create and ref_count is now %i", name, ref.reference_count);

        } else {
            LOG_TRACE("Texture '%s' already exists, ref_count increased to %i", name, ref.reference_count);
        }

        hashtable_set(&state_ptr->registered_texture_table, name, &ref);
        return &state_ptr->registered_textures[ref.handle];
    }

    LOG_ERROR("texture system acquire failed to acquire texture '%s'. null pointer will be returned", name);
    return 0;
}

void texture_system_release(const char *name) {
    // Ignore release requests for the defautl texture
    if (string_equals_case(name, DEFAULT_TEXTURE_NAME)) {
        return;
    }
    texture_reference ref;
    if (state_ptr && hashtable_get(&state_ptr->registered_texture_table, name, &ref)) {
        if (ref.reference_count == 0) {
            LOG_WARN("tried to release non-existant texture: '%s'", name);
            return;
        }
        ref.reference_count--;
        if (ref.reference_count == 0 && ref.auto_release) {
            texture* t = &state_ptr->registered_textures[ref.handle];

            renderer_destroy_texture(t);

            czero_memory(t, sizeof(texture));
            t->id = INVALID_ID;
            t->generation = INVALID_ID;

            // Reset the reference
            ref.handle = INVALID_ID;
            ref.auto_release = false;
            LOG_TRACE("Released texture '%s' and ref_count is now %i", name, ref.reference_count);
        } else {
            LOG_TRACE("Released texture '%s', now ref_count is %i", name, ref.reference_count);
        }

        hashtable_set(&state_ptr->registered_texture_table, name, &ref);
    } else {
        LOG_ERROR("texture failed to release texture '%s'", name);
    }
}

texture* texture_system_get_default_texture() {
    if (state_ptr) {
        return &state_ptr->default_texture;
    }

    LOG_ERROR("texture system not initialized. cannot get default texture");
    return 0;
}


void create_texture(texture* texture) {
    czero_memory(texture, sizeof(texture));
    texture->generation = INVALID_ID;
}

b8 load_texture(const char* texture_name, texture* t) {
    char file_path[1024];
    string_format(file_path, "./assets/textures/%s.png", texture_name);

    const i32 required_channel_count = 4;
    stbi_set_flip_vertically_on_load(true);
    texture temp_texture; // making a temp texture because t can be a existing texture
    u8* data = stbi_load(
        file_path,
        (i32*)&temp_texture.width,
        (i32*)&temp_texture.height,
        (i32*)&temp_texture.channel_count,
        required_channel_count);
    if (!data) {
        LOG_ERROR("Failed to load texture %s. %s", file_path, stbi_failure_reason());
        return false;
    }
    LOG_DEBUG("Loaded texture %s (%d x %d)", file_path, temp_texture.width, temp_texture.height);

    temp_texture.channel_count = required_channel_count;
    u32 current_generation = t->generation;
    t->generation = INVALID_ID;

    u64 total_size = temp_texture.width * temp_texture.height * temp_texture.channel_count;
    // check if any transparency
    b8 has_transparency = false;
    for (u64 i = 0; i < total_size; i += temp_texture.channel_count) {
        u8 a = data[i + 3];
        if (a < 255) {
            has_transparency = true;
            break;
        }
    }

    if (stbi_failure_reason()) {
        LOG_WARN("Failed to load texture %s: %s", file_path, stbi_failure_reason());
    }

    renderer_create_texture(
        texture_name,
        temp_texture.width,
        temp_texture.height,
        temp_texture.channel_count,
        data,
        has_transparency,
        &temp_texture);

    texture old = *t;
    *t = temp_texture;

    renderer_destroy_texture(&old);

    if (current_generation == INVALID_ID) {
        t->generation = 0;
    } else {
        t->generation = current_generation + 1; // because of the texture change, we increment the generation
    }

    stbi_image_free(data);
    return true;
}

b8 create_default_texture(texture *texture) {
    // Generate a default texture
    LOG_TRACE("Generating default texture...");
    const u32 tex_dimension = 256;
    const u32 channels = 4;
    const u32 pixel_count = tex_dimension * tex_dimension;
    u8 pixels[pixel_count * channels];
    cset_memory(pixels, 255, sizeof(u8) * pixel_count * channels);

    // each pixel is a 4 byte value (RGBA)
    for (u64 row = 0; row < tex_dimension; ++row) {
        for (u64 col = 0; col < tex_dimension; ++col) {
            u64 index = (row * tex_dimension) + col;
            u64 index_bpp = index * channels;
            if (row % 2) {
                if (col % 2) {
                    pixels[index_bpp + 1] = 0;
                    pixels[index_bpp + 2] = 0;
                }
            } else {
                if (!(col % 2)) {
                    pixels[index_bpp + 1] = 0;
                    pixels[index_bpp + 2] = 0;
                }
            }
        }
    }

    renderer_create_texture(DEFAULT_TEXTURE_NAME, tex_dimension, tex_dimension, channels, pixels, false, texture);
    texture->generation = INVALID_ID;

    return true;
}

void destroy_default_texture(texture_system_state* state) {
    if (state) {
        renderer_destroy_texture(&state->default_texture);
    }
}