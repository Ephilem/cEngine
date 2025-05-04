#include "text_loader.h"

#include "platform/filesystem.h"
#include "core/cstring.h"
#include "core/logger.h"
#include "core/cmemory.h"
#include "systems/resource_system.h"

b8 text_loader_load(struct resource_loader* self, const char* name, resource* out) {
    if (!self || !name || !out) {
        return false;
    }

    char* format_str = "%s/%s/%s%s";
    char full_file_path[512];
    string_format(full_file_path, format_str, resource_system_base_path(), self->type_path, name, ".txt");

    out->full_path = string_duplicate(full_file_path);

    // load the file
    file_handle file;
    if (!filesystem_open(full_file_path, FILE_MODE_READ, false, &file)) {
        LOG_ERROR("Failed to open text file '%s'", full_file_path);
        return false;
    }

    // Read file
    u64 size = 0;
    if (!filesystem_size(&file, &size)) {
        LOG_ERROR("Failed to get size of text file '%s'", full_file_path);
        filesystem_close(&file);
        return false;
    }

    char* data = callocate(size + 1, MEMORY_TAG_ARRAY);
    if (!data) {
        LOG_ERROR("Failed to allocate memory for text file '%s'", full_file_path);
        filesystem_close(&file);
        return false;
    }

    u64 bytes_read = 0;
    if (!filesystem_read(&file, size, data, &bytes_read) || bytes_read != size) {
        LOG_ERROR("Failed to read text file '%s'", full_file_path);
        cfree(data, size + 1, MEMORY_TAG_ARRAY);
        filesystem_close(&file);
        return false;
    }

    data[size] = '\0'; // null-terminate the string

    filesystem_close(&file);

    out->data = data;
    out->data_size = size;
    out->name = name;

    return true;
}

void text_loader_unload(struct resource_loader* self, resource* resource) {
    if (!self || !resource) {
        return;
    }

    if (resource->full_path) {
        cfree(resource->full_path, sizeof(char) * string_length(resource->full_path) + 1, MEMORY_TAG_STRING);
    }

    if (resource->data) {
        cfree(resource->data, resource->data_size + 1, MEMORY_TAG_ARRAY);
        resource->data = 0;
        resource->data_size = 0;
        resource->loader_id = INVALID_ID;
    }
}

resource_loader text_resource_loader_create() {
    resource_loader resource_loader;
    resource_loader.type = RESOURCE_TYPE_TEXT;
    resource_loader.type_path = "text";
    resource_loader.load = text_loader_load;
    resource_loader.unload = text_loader_unload;
    resource_loader.custom_type = 0;

    return resource_loader;
}

