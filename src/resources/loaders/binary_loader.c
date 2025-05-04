#include "binary_loader.h"

#include "core/cstring.h"
#include "core/logger.h"
#include "core/cmemory.h"
#include "platform/filesystem.h"
#include "systems/resource_system.h"

b8 binary_loader_load(struct resource_loader* self, const char* name, resource* out) {
    if (!self || !name || !out) {
        return false;
    }

    char* format_str = "%s/%s/%s%s";
    char full_file_path[512];
    string_format(full_file_path, format_str, resource_system_base_path(), self->type_path, name, "");

    out->full_path = string_duplicate(full_file_path);

    // load the file
    file_handle file;
    if (!filesystem_open(full_file_path, FILE_MODE_READ, true, &file)) {
        LOG_ERROR("Failed to open binary file '%s'", full_file_path);
        return false;
    }

    // Read file
    u64 size = 0;
    if (!filesystem_size(&file, &size)) {
        LOG_ERROR("Failed to get size of binary file '%s'", full_file_path);
        filesystem_close(&file);
        return false;
    }

    u8* data = callocate(size, MEMORY_TAG_ARRAY);
    if (!data) {
        LOG_ERROR("Failed to allocate memory for binary file '%s'", full_file_path);
        filesystem_close(&file);
        return false;
    }

    u64 bytes_read = 0;
    if (!filesystem_read(&file, size, data, &bytes_read) || bytes_read != size) {
        LOG_ERROR("Failed to read binary file '%s'", full_file_path);
        cfree(data, size, MEMORY_TAG_ARRAY);
        filesystem_close(&file);
        return false;
    }

    filesystem_close(&file);

    out->data = data;
    out->data_size = size;
    out->name = name;

    return true;
}

void binary_loader_unload(struct resource_loader* self, resource* resource) {
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

resource_loader binary_resource_loader_create() {
    resource_loader resource_loader;
    resource_loader.type = RESOURCE_TYPE_BINARY;
    resource_loader.type_path = "";
    resource_loader.load = binary_loader_load;
    resource_loader.unload = binary_loader_unload;
    resource_loader.custom_type = 0;

    return resource_loader;
}