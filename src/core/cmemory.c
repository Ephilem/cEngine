#include "cmemory.h"

#include <stdio.h>
#include <string.h>

#include "core/logger.h"
#include "platform/platform.h"

struct memory_stats {
    u64 total_allocated;
    u64 tagged_allocations[MEMORY_TAG_MAX_TAGS];
};

static const char* memory_tag_strings[MEMORY_TAG_MAX_TAGS] = {
    "UNKNOWN    ",
    "ARRAY      ",
    "DARRAY     ",
    "DICT       ",
    "RING_QUEUE ",
    "BST        ",
    "STRING     ",
    "APPLICATION",
    "JOB        ",
    "RENDERER   ",
    "VULKAN     ",
};

static struct memory_stats stats;

void initialize_memory() {
    platform_zero_memory(&stats, sizeof(stats));
}

void shutdown_memory() {

}

void* callocate(u64 size, memory_tag tag) {
    if (tag == MEMORY_TAG_UNKNOWN) {
        LOG_WARN("Allocating memory with unknown tag is not recommended, try to use a more specific tag");
    }

    stats.total_allocated += size;
    stats.tagged_allocations[tag] += size;

    // TODO: memory allignment
    void* block = platform_allocate(size, FALSE);
    platform_zero_memory(block, size); // Force zeroing memory
    return block;
}

void cfree(void* block, u64 size, memory_tag tag) {
    if (tag == MEMORY_TAG_UNKNOWN) {
        LOG_WARN("Freeing memory with unknown tag is not recommended, try to use a more specific tag");
    }

    stats.total_allocated -= size;
    stats.tagged_allocations[tag] -= size;

    platform_free(block, FALSE);
}

void* czero_memory(void* block, u64 size) {
    return platform_zero_memory(block, size);
}

void* ccopy_memory(void* dest, const void* src, u64 size) {
    return platform_copy_memory(dest, src, size);
}

void* cset_memory(void* dest, i32 value, u64 size) {
    return platform_set_memory(dest, value, size);
}

char* get_memory_usage_str() {
    const u64 gib = 1024 * 1024 * 1024;
    const u64 mib = 1024 * 1024;
    const u64 kib = 1024;

    char buffer[8000] = "System Memory Usage:\n";
    u64 offset = strlen(buffer);

    for (u32 i = 0; i < MEMORY_TAG_MAX_TAGS; ++i) {
        char unit[4] = "Xib";
        float amount = 1.0f;
        if (stats.tagged_allocations[i] >= gib) {
            amount = (float)stats.tagged_allocations[i] / (float)gib;
            unit[0] = 'G';
        } else if (stats.tagged_allocations[i] > mib) {
            amount = (float)stats.tagged_allocations[i] / (float)mib;
            unit[0] = 'M';
        } else if (stats.tagged_allocations[i] > kib) {
            amount = (float)stats.tagged_allocations[i] / (float)kib;
            unit[0] = 'K';
        } else {
            amount = (float)stats.tagged_allocations[i];
            unit[0] = 'B';
            unit[1] = 0;
        }

        i32 length = snprintf(buffer + offset, 8000, "  %s: %.2f %s\n", memory_tag_strings[i], amount, unit);
        offset += length;
    }

    char* out_string = strdup(buffer); // do allocation here
    return out_string;
}
