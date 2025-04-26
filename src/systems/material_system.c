#include "material_system.h"

#include "core/logger.h"
#include "core/cmemory.h"
#include "core/cstring.h"
#include "containers/hashtable.h"
#include "renderer/renderer_frontend.h"

// TEMP
#include "texture_system.h"
#include "math/cmath.h"
#include "platform/filesystem.h"

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
b8 load_configuration_file(const char* file_path, material_config* config);

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
    material_config config;

    // load file from disk
    // TODO : should be able to be localed anywhere
    char* format_str = "./assets/materials/%s.%s";
    char file_path[1024];

    string_format(file_path, format_str, name, "cmat");
    if (!load_configuration_file(file_path, &config)) {
        LOG_ERROR("Failed to load material configuration file %s", file_path);
        return 0;
    }

    return material_system_acquire_from_config(config);
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

b8 load_configuration_file(const char* file_path, material_config* config) {
    czero_memory(config, sizeof(material_config));

    // load the file
    file_handle file;
    if (!filesystem_open(file_path, FILE_MODE_READ, false, &file)) {
        LOG_ERROR("Failed to open material configuration file '%s'", file_path);
        return false;
    }

    // Read file
    char line_buffer[1024] = "";
    char* p = &line_buffer[0];
    u64 line_length = 0;
    u32 line_number = 1;
    while (filesystem_read_line(&file, 1023, &p, &line_length)) {
        char* trimmed = string_trim(line_buffer);
        line_length = string_length(trimmed);

        // skip empty lines and comments
        if (line_length == 0 || trimmed[0] == '#') {
            line_number++;
            continue;
        }

        // parse the line
        i32 equal_index = string_index_of(trimmed, '=');
        if (equal_index == -1) {
            LOG_WARN("Invalid line in material configuration file '%s' at line %i: '%s'", file_path, line_number, trimmed);
            line_number++;
            continue;
        }

        // assume the key max length is 64
        char raw_var_name[64] = "";
        czero_memory(raw_var_name, sizeof(char) * 64);
        string_mid(raw_var_name, trimmed, 0, equal_index);
        char* trimmed_var_name = string_trim(raw_var_name);

        // assume a max of 511 - 65 = 446 for the value
        char raw_value[446];
        czero_memory(raw_value, sizeof(char) * 446);
        string_mid(raw_value, trimmed, equal_index + 1, -1);
        char* trimmed_var_value = string_trim(raw_value);

        // process the variable
        if (string_equals_case(trimmed_var_name, "version")) {
            // TODO: versioning
        } else if (string_equals_case(trimmed_var_name, "name")) {
            string_ncopy(config->name, trimmed_var_value, MATERIAL_NAME_MAX_LENGTH);
        } else if (string_equals_case(trimmed_var_name, "diffuse_map_name")) {
            string_ncopy(config->diffuse_map_name, trimmed_var_value, TEXTURE_NAME_MAX_LENGTH);
        } else if (string_equals_case(trimmed_var_name, "diffuse_color")) {
            if (!string_to_vec4(trimmed_var_value, &config->diffuse_color)) {
                LOG_WARN("Invalid diffuse color in material configuration file '%s' at line %i: '%s'", file_path, line_number, trimmed_var_value);
                config->diffuse_color = vec4_one();
            }
        }

        // TODO more fields

        czero_memory(line_buffer, sizeof(char) * line_length);
        line_number++;
    }

    // close file
    filesystem_close(&file);

    return true;
}

material* material_system_get_default_material() {
    if (state_ptr) {
        return &state_ptr->default_material;
    }

    LOG_FATAL("Material system not initialized. Cannot get default material.");
    return 0;
}
