#include "linear_allocator.h"

#include "core/cmemory.h"
#include "core/logger.h"

void linear_allocator_create(u64 total_size, void *memory, linear_allocator *out_allocator) {
    if (!out_allocator) {
        LOG_ERROR("linear_allocator_create - Invalid allocator pointer provided!");
        return;
    }
    
    out_allocator->total_size = total_size;
    out_allocator->allocated = 0;
    out_allocator->owns_memory = memory == 0;

    if (memory) {
        out_allocator->memory = memory;
    } else {
        out_allocator->memory = callocate(total_size, MEMORY_TAG_LINEAR_ALLOCATOR);
        if (!out_allocator->memory) {
            LOG_ERROR("linear_allocator_create - Failed to allocate %llu bytes of memory for linear allocator!", total_size);
            return;
        }
    }
}

void linear_allocator_destroy(linear_allocator *allocator) {
    if (allocator) {
        allocator->total_size = 0;
        allocator->allocated = 0;
        if (allocator->owns_memory && allocator->memory) {
            cfree(allocator->memory, allocator->total_size, MEMORY_TAG_LINEAR_ALLOCATOR);
        } else {
            allocator->memory = 0;
        }
        allocator->total_size = 0;
        allocator->owns_memory = false;
    }
}

void* linear_allocator_allocate(linear_allocator* allocator, u64 size) {
    if (allocator && allocator->memory) {
        if (allocator->allocated + size > allocator->total_size) {
            u64 remaining = allocator->total_size - allocator->allocated;
            LOG_ERROR("linear_allocator_allocate - Tried to allocate %lluB, only %lluB remaining.", size, remaining);
            return 0;
        }

        void* block = ((u8*)allocator->memory) + allocator->allocated;
        allocator->allocated += size;
        return block;
    }

    LOG_ERROR("linear_allocator_allocate - provided allocator not initialized.");
    return 0;
}


void linear_allocator_free_all(linear_allocator *allocator) {
    if (allocator && allocator->memory) {
        allocator->allocated = 0;
        czero_memory(allocator->memory, allocator->total_size);
    }
}
