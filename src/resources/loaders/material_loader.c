#include "material_loader.h"

#include "core/logger.h"
#include "core/cmemory.h"
#include "core/cstring.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"
#include "math/cmath.h"

#include "platform/filesystem.h"

b8 material_loader_load(struct resource_loader* self, const char* name, resource* out) {
    if (!self || !name || !out) {
        return false;
    }

    char* format_str = "%s/%s/%s%s";
    char full_file_path[512];
    string_format(full_file_path, format_str, resource_system_base_path(), self->type_path, name, ".cmat");

    out->full_path = string_duplicate(full_file_path);

    // load the file
    file_handle file;
    if (!filesystem_open(full_file_path, FILE_MODE_READ, false, &file)) {
        LOG_ERROR("Failed to open material configuration file '%s'", full_file_path);
        return false;
    }

    material_config* resource_data = callocate(sizeof(material_config), MEMORY_TAG_UNKNOWN);
    resource_data->auto_release = true;
    resource_data->diffuse_color = vec4_one();
    resource_data->diffuse_map_name[0] = 0;
    string_ncopy(resource_data->name, name, MATERIAL_NAME_MAX_LENGTH);

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
            LOG_WARN("Invalid line in material configuration file '%s' at line %i: '%s'", full_file_path, line_number, trimmed);
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
            string_ncopy(resource_data->name, trimmed_var_value, MATERIAL_NAME_MAX_LENGTH);
        } else if (string_equals_case(trimmed_var_name, "diffuse_map_name")) {
            string_ncopy(resource_data->diffuse_map_name, trimmed_var_value, TEXTURE_NAME_MAX_LENGTH);
        } else if (string_equals_case(trimmed_var_name, "diffuse_color")) {
            if (!string_to_vec4(trimmed_var_value, &resource_data->diffuse_color)) {
                LOG_WARN("Invalid diffuse color in material configuration file '%s' at line %i: '%s'", full_file_path, line_number, trimmed_var_value);
            }
        }

        // TODO more fields

        czero_memory(line_buffer, sizeof(char) * line_length);
        line_number++;
    }

    // close file
    filesystem_close(&file);

    out->data = resource_data;
    out->data_size = sizeof(material_config);
    out->name = name;

    return true;
}

void material_loader_unload(struct resource_loader* self, resource* resource) {
    if (!self || !resource) {
        return;
    }

    u32 path_length = string_length(resource->full_path);
    if (path_length) {
        cfree(resource->full_path, sizeof(char) * path_length + 1, MEMORY_TAG_STRING);
    }

    if (resource->data) {
        cfree(resource->data, resource->data_size, MEMORY_TAG_UNKNOWN);
        resource->data = 0;
        resource->data_size = 0;
        resource->loader_id = INVALID_ID;
    }
}

resource_loader material_resource_loader_create() {
    resource_loader resource_loader;
    resource_loader.type = RESOURCE_TYPE_MATERIAL;
    resource_loader.type_path = "materials";
    resource_loader.load = material_loader_load;
    resource_loader.unload = material_loader_unload;
    resource_loader.custom_type = 0;

    return resource_loader;
}

