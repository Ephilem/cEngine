cmake_minimum_required(VERSION 3.30)
project(cEngine_example C)

set(CMAKE_C_STANDARD 11)

add_executable(cEngine_example
        src/main.c
        src/game.h
        src/game.c
)

target_link_libraries(cEngine_example PRIVATE cEngine)