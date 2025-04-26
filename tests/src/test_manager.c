#include "test_manager.h"

#include <containers/darray.h>
#include <core/logger.h>
#include <core/cstring.h>
#include <core/clock.h>

#include "memory/linear_allocator.h"

typedef struct test_entry {
    PFN_test func;
    char* desc;
} test_entry;

static test_entry* tests;

static linear_allocator systems_allocator;
static u64 logging_system_memory_requirement;
static void* logging_system_state;

void test_manager_init() {
    linear_allocator_create(1024 *  1024, 0, &systems_allocator);

    initialize_logging(&logging_system_memory_requirement, 0);
    logging_system_state = linear_allocator_allocate(&systems_allocator, logging_system_memory_requirement);
    if (!initialize_logging(&logging_system_memory_requirement, logging_system_state)) {
        LOG_FATAL("Failed to initialize logging system! Shutting down.");
        return;
    }
    tests = darray_create(test_entry);
}

void test_manager_register_test(u8 (*PFN_test)(), char* desc) {
    test_entry e;
    e.func = PFN_test;
    e.desc = desc;
    darray_push(tests, e);
}

void test_manager_run_tests() {
    u32 passed = 0;
    u32 failed = 0;
    u32 skipped = 0;

    u32 count = darray_length(tests);

    clock total_time;
    clock_start(&total_time);

    for (u32 i = 0; i < count; ++i) {
        clock test_time;
        clock_start(&test_time);
        u8 result = tests[i].func();
        clock_update(&test_time);

        if (result == true) {
            ++passed;
        } else if (result == BYPASS) {
            LOG_WARN("[SKIPPED]: %s", tests[i].desc);
            ++skipped;
        } else {
            LOG_ERROR("[FAILED]: %s", tests[i].desc);
            ++failed;
        }
        char status[20];
        string_format(status, failed ? "*** %d FAILED ***" : "SUCCESS", failed);
        clock_update(&total_time);
        LOG_INFO("Executed %d of %d (skipped %d) %s (%.6f sec / %.6f sec total) - %s", i + 1, count, skipped, status, test_time.elasped_time, total_time.elasped_time, tests[i].desc);
    }

    clock_stop(&total_time);

    LOG_INFO("Results: %d passed, %d failed, %d skipped.", passed, failed, skipped);
}