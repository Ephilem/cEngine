#include "material_system.h"

#include "core/logger.h"
#include "core/cmemory.h"
#include "core/cstring.h"
#include "containers/hashtable.h"
#include "renderer/renderer_frontend.h"

// TEMP
#include "resource_system.h"
#include "texture_system.h"
#include "math/cmath.h"

typedef struct material_system_state {
    material_system_config config;

    material default_material;

    material* registered_materials;

    hashtable registered_material_table;
} material_system_state;

typedef struct material_reference {
    u64 reference_count;
    u32 handle;
    b8 auto_release;
} material_reference;

static material_system_state* state_ptr = 0;

b8 create_default_material(material_system_state* state);
b8 load_material(material_config config, material* m);
void destroy_material(material* m);

b8 material_system_initialize(u64* memory_requirement, void* state, material_system_config config) {
    if (config.max_material_count == 0) {
        LOG_FATAL("Can't initialize material system with 0 max material count");
        return false;
    }

    // memory requirement
    u64 struct_requirement = sizeof(material_system_state);
    u64 array_requirement = sizeof(material) * config.max_material_count;
    u64 hashtable_requirement = sizeof(material_reference) * config.max_material_count;
    *memory_requirement = struct_requirement + array_requirement + hashtable_requirement;

    if (!state) {
        return true;
    }

    state_ptr = state;
    state_ptr->config = config;

    void* array_block = state + struct_requirement;
    state_ptr->registered_materials = array_block;

    void* hashtable_block = state + struct_requirement;
    hashtable_create(sizeof(material_reference), config.max_material_count, hashtable_block, false, &state_ptr->registered_material_table);

    material_reference invalid_ref;
    invalid_ref.auto_release = false;
    invalid_ref.handle = INVALID_ID;
    invalid_ref.reference_count = 0;

    hashtable_fill(&state_ptr->registered_material_table, &invalid_ref);

    // invalidate all materials in the array
    u32 count = state_ptr->config.max_material_count;
    for (u32 i = 0; i < count; ++i) {
        state_ptr->registered_materials[i].id = INVALID_ID;
        state_ptr->registered_materials[i].generation = INVALID_ID;
        state_ptr->registered_materials[i].internal_id = INVALID_ID;
    }

    if (!create_default_material(state_ptr)) {
        LOG_FATAL("Can't create default material");
        return false;
    }

    LOG_INFO("Material system initialized with %i materials", state_ptr->config.max_material_count);
    return true;
}

void material_system_shutdown(void* state) {
    material_system_state* s = (material_system_state*)state;
    if (s) {
        // invalidate all materials in the array
        u32 count = s->config.max_material_count;
        for (u32 i = 0; i < count; ++i) {
            if (s->registered_materials[i].id != INVALID_ID) {
                destroy_material(&s->registered_materials[i]);
            }
        }

        destroy_material(&s->default_material);
    }

    state_ptr = 0;
}

material* material_system_acquire(const char* name) {
    // load the resource
    resource material_resource;
    if (!resource_system_load(name, RESOURCE_TYPE_MATERIAL, &material_resource)) {
        LOG_ERROR("Failed to load material '%s'", name);
        return 0;
    }

    material* m;
    if (material_resource.data) {
        m = material_system_acquire_from_config(*(material_config*)material_resource.data);
    }

    // cleanup
    resource_system_unload(&material_resource);

    if (!m) {
        LOG_ERROR("Failed to acquire material '%s'", name);
    }

    return m;
}

material* material_system_acquire_from_config(material_config config) {
    if (string_equals_case(config.name, DEFAULT_MATERIAL_NAME)) {
        return &state_ptr->default_material;
    }

    material_reference ref;
    if (state_ptr && hashtable_get(&state_ptr->registered_material_table, config.name, &ref)) {
        // can be cahnged the first time a material is loaded
        if (ref.reference_count == 0) {
            ref.auto_release = config.auto_release;
        }

        ref.reference_count++;

        // if the material is not loaded yet, load it
        if (ref.handle == INVALID_ID) {
            // find a free index in the array
            u32 count = state_ptr->config.max_material_count;
            material* m = 0;
            for (u32 i = 0; i < count; ++i) {
                if (state_ptr->registered_materials[i].id == INVALID_ID) {
                    ref.handle = i;
                    m = &state_ptr->registered_materials[i];
                    break;
                }
            }

            // make sure we found a free index
            if (!m || ref.handle == INVALID_ID) {
                LOG_FATAL("Material system cannot hold anymore materials. adjust configuration to allow more materials");
                return 0;
            }

            // create new material
            if (!load_material(config, m)) {
                LOG_ERROR("Failed to load material '%s'", config.name);
                return 0;
            }

            if (m->generation == INVALID_ID) {
                m->generation = 0;
            } else {
                m->generation++;
            }

            m->id = ref.handle;
            LOG_TRACE("Material '%s' does not yet exist. Create and ref_count is now %i", config.name, ref.reference_count);
        } else {
            LOG_TRACE("Material '%s' already exists. ref_count is now %i", config.name, ref.reference_count);
        }

        hashtable_set(&state_ptr->registered_material_table, config.name, &ref);
        return &state_ptr->registered_materials[ref.handle];
    }

    LOG_ERROR("Failed to acquire material reference '%s'", config.name);
    return 0;
}

void material_system_release(const char* name) {
    if (string_equals_case(name, DEFAULT_MATERIAL_NAME)) {
        return;
    }

    material_reference ref;
    if (state_ptr && hashtable_get(&state_ptr->registered_material_table, name, &ref)) {
        if (ref.reference_count == 0) {
            LOG_WARN("tried to release non-existant material: '%s'", name);
            return;
        }

        ref.reference_count--;

        if (ref.reference_count == 0 && ref.auto_release) {
            material* m = &state_ptr->registered_materials[ref.handle];

            // destroy the material
            destroy_material(m);

            // reset the reference
            ref.handle = INVALID_ID;
            ref.auto_release = false;
            LOG_TRACE("Released material '%s' and ref_count is now %i", name, ref.reference_count);
        } else {
            LOG_TRACE("Released material '%s', now ref_count is %i", name, ref.reference_count);
        }

        hashtable_set(&state_ptr->registered_material_table, name, &ref);
    } else {
        LOG_ERROR("Failed to release material '%s'", name);
    }
}

b8 create_default_material(material_system_state* state) {
    czero_memory(&state->default_material, sizeof(material));
    state->default_material.id = INVALID_ID;
    state->default_material.generation = INVALID_ID;
    string_ncopy(state->default_material.name, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
    state->default_material.diffuse_color = vec4_create(1.0f, 1.0f, 1.0f, 1.0f); // white
    state->default_material.diffuse_map.use = TEXTURE_USE_MAP_DIFFUSE;
    state->default_material.diffuse_map.texture = texture_system_get_default_texture();

    if (!renderer_create_material(&state->default_material)) {
        LOG_FATAL("Failed to create default material");
        return false;
    }

    return true;
}

b8 load_material(material_config config, material* m) {
    czero_memory(m, sizeof(material));

    // name
    string_ncopy(m->name, config.name, MATERIAL_NAME_MAX_LENGTH);

    // diffuse color
    m->diffuse_color = config.diffuse_color;

    // diffuse map
    if (string_length(config.diffuse_map_name) > 0) {
        m->diffuse_map.use = TEXTURE_USE_MAP_DIFFUSE;
        m->diffuse_map.texture = texture_system_acquire(config.diffuse_map_name, true);
        if (!m->diffuse_map.texture) {
            LOG_ERROR("Failed to load diffuse map '%s' for material '%s'", config.diffuse_map_name, m->name);
            return false;
        }
    } else {
        // only set for clarity, as call to czero_memory will set it to 0 already
        m->diffuse_map.use = TEXTURE_USE_UNKNOWN;
        m->diffuse_map.texture = 0;
    }

    // TODO: other maps (normal, specular, etc)

    // Send it off to the renderer
    if (!renderer_create_material(m)) {
        LOG_ERROR("Failed to acquire renderer resource for material '%s'", m->name);
        return false;
    }

    return true;
}

void destroy_material(material* m) {
    LOG_TRACE("destroying material '%s'", m->name);

    if (m->diffuse_map.texture) {
        texture_system_release(m->diffuse_map.texture->name);
    }

    renderer_destroy_material(m);
    czero_memory(m, sizeof(material));
    m->id = INVALID_ID;
    m->generation = INVALID_ID;
    m->internal_id = INVALID_ID;
}

material* material_system_get_default_material() {
    if (state_ptr) {
        return &state_ptr->default_material;
    }

    LOG_FATAL("Material system not initialized. Cannot get default material.");
    return 0;
}
