cmake_minimum_required(VERSION 3.30)
project(cEngine_example C)

set(CMAKE_C_STANDARD 11)

add_executable(cEngine_example
        src/main.c
        src/game.h
        src/game.c
)

target_link_libraries(cEngine_example PRIVATE cEngine)

# Create symlink to assets folder
add_custom_command(
    TARGET cEngine_example POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_CURRENT_SOURCE_DIR}/../assets
    ${CMAKE_CURRENT_BINARY_DIR}/assets
    COMMENT "Creating symlink to assets directory in example project"
)