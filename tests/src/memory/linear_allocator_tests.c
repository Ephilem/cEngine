#include "linear_allocator_tests.h"

#include <memory/linear_allocator.h>
#include "../test_manager.h"
#include "../expect.h"
#include <core/logger.h>
#include <core/cmemory.h>

// Test la création d'un allocateur linéaire avec auto-allocation
u8 test_linear_allocator_create() {
    linear_allocator allocator;
    u64 size = 1024;

    linear_allocator_create(size, 0, &allocator);

    expect_should_be(size, allocator.total_size);
    expect_should_be(0, allocator.allocated);
    expect_to_be_true(allocator.memory != 0);
    expect_to_be_true(allocator.owns_memory);

    linear_allocator_destroy(&allocator);
    
    return true;
}

u8 test_linear_allocator_create_with_memory() {
    linear_allocator allocator;
    u64 size = 1024;
    void* memory = callocate(size, MEMORY_TAG_UNKNOWN);

    linear_allocator_create(size, memory, &allocator);

    expect_should_be(size, allocator.total_size);
    expect_should_be(0, allocator.allocated);
    expect_should_be(memory, allocator.memory);
    expect_to_be_false(allocator.owns_memory);

    linear_allocator_destroy(&allocator);
    cfree(memory, size, MEMORY_TAG_UNKNOWN);
    
    return true;
}

u8 test_linear_allocator_allocate() {
    linear_allocator allocator;
    u64 size = 1024;

    linear_allocator_create(size, 0, &allocator);

    u64 alloc_size_1 = 256;
    void* block_1 = linear_allocator_allocate(&allocator, alloc_size_1);

    expect_to_be_true(block_1 != 0);
    expect_should_be(alloc_size_1, allocator.allocated);
    expect_should_be(allocator.memory, block_1);

    u64 alloc_size_2 = 128;
    void* block_2 = linear_allocator_allocate(&allocator, alloc_size_2);

    expect_to_be_true(block_2 != 0);
    expect_should_be(alloc_size_1 + alloc_size_2, allocator.allocated);
    expect_should_be((u8*)allocator.memory + alloc_size_1, block_2);

    linear_allocator_destroy(&allocator);
    
    return true;
}

u8 test_linear_allocator_out_of_memory() {
    linear_allocator allocator;
    u64 size = 1024;

    linear_allocator_create(size, 0, &allocator);

    u64 alloc_size_1 = 900;
    void* block_1 = linear_allocator_allocate(&allocator, alloc_size_1);

    expect_to_be_true(block_1 != 0);
    expect_should_be(alloc_size_1, allocator.allocated);

    u64 alloc_size_2 = 200;
    void* block_2 = linear_allocator_allocate(&allocator, alloc_size_2);

    expect_to_be_true(block_2 == 0);
    expect_should_be(alloc_size_1, allocator.allocated);

    linear_allocator_destroy(&allocator);
    
    return true;
}

u8 test_linear_allocator_free_all() {
    linear_allocator allocator;
    u64 size = 1024;

    linear_allocator_create(size, 0, &allocator);

    u64 alloc_size = 512;
    void* block = linear_allocator_allocate(&allocator, alloc_size);

    expect_to_be_true(block != 0);
    expect_should_be(alloc_size, allocator.allocated);

    linear_allocator_free_all(&allocator);

    expect_should_be(0, allocator.allocated);

    void* new_block = linear_allocator_allocate(&allocator, alloc_size);

    expect_to_be_true(new_block != 0);
    expect_should_be(alloc_size, allocator.allocated);
    expect_should_be(allocator.memory, new_block);

    linear_allocator_destroy(&allocator);
    
    return true;
}

u8 test_linear_allocator_destroy_owned() {
    linear_allocator allocator;
    u64 size = 1024;

    linear_allocator_create(size, 0, &allocator);

    expect_to_be_true(allocator.owns_memory);
    expect_to_be_true(allocator.memory != 0);

    linear_allocator_destroy(&allocator);

    expect_to_be_false(allocator.owns_memory);
    expect_should_be(0, allocator.total_size);
    expect_should_be(0, allocator.allocated);
    
    return true;
}

u8 test_linear_allocator_destroy_not_owned() {
    linear_allocator allocator;
    u64 size = 1024;
    void* memory = callocate(size, MEMORY_TAG_UNKNOWN);

    linear_allocator_create(size, memory, &allocator);

    expect_to_be_false(allocator.owns_memory);
    expect_should_be(memory, allocator.memory);

    linear_allocator_destroy(&allocator);

    expect_to_be_false(allocator.owns_memory);
    expect_should_be(0, allocator.memory);
    expect_should_be(0, allocator.total_size);
    expect_should_be(0, allocator.allocated);

    cfree(memory, size, MEMORY_TAG_UNKNOWN);
    
    return true;
}

void linear_allocator_register_tests() {
    test_manager_register_test(test_linear_allocator_create, "Linear Allocator creation with auto-allocation");
    test_manager_register_test(test_linear_allocator_create_with_memory, "Linear Allocator creation with provided memory");
    test_manager_register_test(test_linear_allocator_allocate, "Linear Allocator memory allocation");
    test_manager_register_test(test_linear_allocator_out_of_memory, "Linear Allocator out of memory handling");
    test_manager_register_test(test_linear_allocator_free_all, "Linear Allocator free all memory");
    test_manager_register_test(test_linear_allocator_destroy_owned, "Linear Allocator destruction (owned memory)");
    test_manager_register_test(test_linear_allocator_destroy_not_owned, "Linear Allocator destruction (not owned memory)");
}
