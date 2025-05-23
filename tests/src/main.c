#include "test_manager.h"
#include "memory/linear_allocator_tests.h"
#include "containers/hashtable_tests.h"
#include "core/cstring_tests.h"

#include <core/logger.h>

int main() {
    test_manager_init();

    linear_allocator_register_tests();
    hashtable_register_tests();
    cstring_register_tests();

    LOG_INFO("Starting tests...");

    test_manager_run_tests();

    return 0;
}