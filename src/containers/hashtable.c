#include "hashtable.h"

#include "core/logger.h"
#include "core/cmemory.h"
#include "core/cstring.h"
#include "renderer/vulkan/vulkan_image.h"

u64 hash_name(const char* name, u32 element_count) {
    // A multiplier to use when generating a hash. Prime to hopefully avoid collisions.
    static const u64 multiplier = 97;

    unsigned const char* us;
    u64 hash = 0;

    for (us = (unsigned const char*)name; *us; us++) {
        hash = hash * multiplier + *us;
    }

    // Mod it agaist the size of the table
    hash = hash % element_count;

    return hash;
}

void hashtable_create(u64 element_size, u32 element_count, void* memory, b8 is_pointer_type, hashtable* out_table) {
    if (!memory || !out_table) {
        LOG_ERROR("Invalid memory or out_table pointer");
        return;
    }
    if (!element_count || !element_size) {
        LOG_ERROR("Invalid element count or size");
        return;
    }

    // TODO : mayby use an allocator and allocate the memory here
    out_table->memory = memory;
    out_table->element_count = element_count;
    out_table->element_size = element_size;
    out_table->is_pointer_type = is_pointer_type;
    czero_memory(out_table->memory, element_size * element_count);
}

void hashtable_destroy(hashtable* table) {
    if (table) {
        // TODO: if using allocator above, free the memory here
        czero_memory(table, sizeof(hashtable));
    }
}

b8 hashtable_set(hashtable* table, const char* name, void* value) {
    if (!table || !name || !value) {
        LOG_ERROR("Invalid table, name or value pointer");
        return false;
    }
    if (table->is_pointer_type) {
        LOG_ERROR("Invalid table, name or value pointer type is not a pointer");
        return false;
    }

    u64 hash = hash_name(name, table->element_count);
    ccopy_memory(table->memory + (table->element_size * hash), value, table->element_size);
    return true;
}

b8 hashtable_set_ptr(hashtable* table, const char* name, void** value) {
    if (!table || !name) {
        LOG_ERROR("Invalid table or name pointer pointer");
        return false;
    }
    if (!table->is_pointer_type) {
        LOG_ERROR("Invalid table, name or value pointer type is not a pointer");
        return false;
    }

    u64 hash = hash_name(name, table->element_count);
    ((void**) table->memory)[hash] = value ? *value : 0;
    return true;
}

b8 hashtable_get(hashtable* table, const char* name, void* out_value) {
    if (!table || !name || !out_value) {
        LOG_ERROR("Invalid table, name or out_value pointer");
        return false;
    }
    if (table->is_pointer_type) {
        LOG_ERROR("Invalid table, name or value pointer type is not a pointer");
        return false;
    }

    u64 hash = hash_name(name, table->element_count);
    ccopy_memory(out_value, table->memory + (table->element_size * hash), table->element_size);
    return true;
}

b8 hashtable_get_ptr(hashtable* table, const char* name, void** out_value) {
    if (!table || !name || !out_value) {
        LOG_ERROR("Invalid table, name or out_value pointer pointer");
        return false;
    }
    if (!table->is_pointer_type) {
        LOG_ERROR("Invalid table, name or value pointer type is not a pointer");
        return false;
    }

    u64 hash = hash_name(name, table->element_count);
    *out_value = ((void**) table->memory)[hash];
    return *out_value != 0;
}

b8 hashtable_fill(hashtable* table, void* value) {
    if (!table || !value) {
        LOG_ERROR("Invalid table or value pointer");
        return false;
    }
    if (table->is_pointer_type) {
        LOG_ERROR("Invalid table, name or value pointer type is not a pointer");
        return false;
    }

    for (u32 i = 0; i < table->element_count; ++i) {
        ccopy_memory(table->memory + (table->element_size * i), value, table->element_size);
    }
    return true;
}