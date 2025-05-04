#include "resource_system.h"

#include "core/logger.h"
#include "core/cstring.h"

// known resource loader
#include "resources/loaders/binary_loader.h"
#include "resources/loaders/image_loader.h"
#include "resources/loaders/material_loader.h"
#include "resources/loaders/text_loader.h"

typedef struct resource_system_state {
    resource_system_config config;
    resource_loader* registered_loaders;
} resource_system_state;

static resource_system_state* state_ptr = 0;

b8 load(const char* name, resource_loader* loader, resource* out_resource);
b8 unload(resource_loader* loader, resource* res);

b8 resource_system_initialize(u64* memory_requirement, void* state, resource_system_config config) {
    if (config.max_loader_count == 0) {
        LOG_ERROR("Resource system: max loader count is 0");
        return false;
    }

    *memory_requirement = sizeof(resource_system_state) + (sizeof(resource_loader) * config.max_loader_count);

    if (!state) {
        return true;
    }

    state_ptr = state;
    state_ptr->config = config;

    void* array_block = state + sizeof(resource_system_state);
    state_ptr->registered_loaders = array_block;

    // invalidate all loaders
    u32 count = config.max_loader_count;
    for (u32 i = 0; i < count; ++i) {
        state_ptr->registered_loaders[i].id = INVALID_ID;
    }

    resource_system_register_loader(image_resource_loader_create());
    resource_system_register_loader(material_resource_loader_create());
    resource_system_register_loader(binary_resource_loader_create());
    resource_system_register_loader(text_resource_loader_create());


    LOG_INFO("Resource system initialized");

    return true;
}

void resource_system_shutdown(void* state) {
    if (state_ptr) {
        state_ptr = 0;
    }
}

b8 resource_system_register_loader(resource_loader loader) {
    if (state_ptr) {
        u32 count = state_ptr->config.max_loader_count;
        // verify if no loader for the given type is already registered
        for (u32 i = 0; i < count; ++i) {
            resource_loader* l = &state_ptr->registered_loaders[i];
            if (l->id != INVALID_ID) {
                if (l->type == loader.type) {
                    LOG_ERROR("Loader of type %d already registered", loader.type);
                    return false;
                }
                if (loader.custom_type && string_length(loader.custom_type) > 0 && string_equals_case(loader.custom_type, l->custom_type)) {
                    LOG_ERROR("Loader of custom type %s already registered", loader.custom_type);
                    return false;
                }
            }
        }
        // search for an empty slot
        for (u32 i = 0; i < count; ++i) {
            if (state_ptr->registered_loaders[i].id == INVALID_ID) {
                state_ptr->registered_loaders[i] = loader;
                state_ptr->registered_loaders[i].id = i;
                return true;
            }
        }
    }

    return false;
}

b8 resource_system_load(const char* name, resource_type type, resource* out_resource) {
    if (state_ptr && type != RESOURCE_TYPE_CUSTOM) {
        u32 count = state_ptr->config.max_loader_count;
        for (u32 i = 0; i < count; ++i) {
            resource_loader* loader = &state_ptr->registered_loaders[i];
            if (loader->id != INVALID_ID && loader->type == type) {
                return load(name, loader, out_resource);
            }
        }
    }

    out_resource->loader_id = INVALID_ID;
    LOG_ERROR("Resource system: no loader registered for type %d", type);
    return false;
}

b8 resource_system_load_custom(const char* name, const char* custom_type, resource* out_resource) {
    if (state_ptr && custom_type && string_length(custom_type) > 0) {
        u32 count = state_ptr->config.max_loader_count;
        for (u32 i = 0; i < count; ++i) {
            resource_loader* loader = &state_ptr->registered_loaders[i];
            if (loader->id != INVALID_ID && loader->type == RESOURCE_TYPE_CUSTOM && string_equals_case(custom_type, loader->custom_type)) {
                return load(name, loader, out_resource);
            }
        }
    }

    out_resource->loader_id = INVALID_ID;
    LOG_ERROR("Resource system: no loader registered for type %s", custom_type);
    return false;
}

void resource_system_unload(resource* res) {
    if (state_ptr) {
        if (res->loader_id != INVALID_ID) {
            resource_loader* loader = &state_ptr->registered_loaders[res->loader_id];
            if (loader->id != INVALID_ID && loader->unload) {
                loader->unload(loader, res);
            }
        }
    }
}

const char* resource_system_base_path() {
    if (state_ptr) {
        return state_ptr->config.asset_base_path;
    }

    LOG_ERROR("Resource system: no base path");
    return 0;
}

b8 load(const char* name, resource_loader* loader, resource* out_resource) {
    if (!name || !loader || !loader->load || !out_resource) {
        out_resource->loader_id = INVALID_ID;
        return false;
    }

    out_resource->loader_id = loader->id;
    return loader->load(loader, name, out_resource);
}

b8 unload(resource_loader* loader, resource* res) {
}

