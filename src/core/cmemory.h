#pragma once

#include "define.h"

typedef enum memory_tag {
    MEMORY_TAG_UNKNOWN,
    MEMORY_TAG_ARRAY,
    MEMORY_TAG_DARRAY,
    MEMORY_TAG_DICT,
    MEMORY_TAG_RING_QUEUE,
    MEMORY_TAG_BST,
    MEMORY_TAG_STRING,
    MEMORY_TAG_APPLICATION,
    MEMORY_TAG_JOB,
    MEMORY_TAG_RENDERER,

    MEMORY_TAG_MAX_TAGS,
} memory_tag;

void initialize_memory();
void shutdown_memory();

void* callocate(u64, memory_tag tag);

void cfree(void* block, u64 size, memory_tag tag);

void* czero_memory(void* block, u64 size);

void* ccopy_memory(void* dest, const void* src, u64 size);

void* cset_memory(void* dest, i32 value, u64 size);

char* get_memory_usage_str();


