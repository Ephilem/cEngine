cmake_minimum_required(VERSION 3.30)
project(cEngine_tests C)

set(CMAKE_C_STANDARD 11)

# Créer l'exécutable de tests
add_executable(cEngine_tests
        src/main.c
        src/expect.h
        src/test_manager.h
        src/test_manager.c
        src/memory/linear_allocator_tests.c
        src/memory/linear_allocator_tests.h
        src/containers/hashtable_tests.c
        src/containers/hashtable_tests.h
        src/core/cstring_tests.c
        src/core/cstring_tests.h
)


target_link_libraries(cEngine_tests PRIVATE cEngine)
