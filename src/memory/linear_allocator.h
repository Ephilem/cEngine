#pragma once

#include "define.h"

typedef struct linear_allocator {
    u64 total_size;
    u64 allocated;
    void* memory;
    b8 owns_memory; // if true, the allocator will free the memory when destroyed
} linear_allocator;

void linear_allocator_create(u64 total_size, void* memory, linear_allocator* allocator);
void linear_allocator_destroy(linear_allocator* allocator);

void* linear_allocator_allocate(linear_allocator* allocator, u64 size);
void linear_allocator_free_all(linear_allocator* allocator);


