cmake_minimum_required(VERSION 3.30)
project(cEngine C)

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
find_package(X11 REQUIRED)
find_package(Vulkan REQUIRED)
pkg_check_modules(XCB REQUIRED xcb)

add_library(cEngine
        src/core/logger.c
        src/core/logger.h
        src/define.h
        src/core/asserts.h
        src/platform/platform.h
        src/platform/platform_linux.c
        src/core/application.h
        src/core/application.c
        src/app.h
        src/entry.h
        src/core/cmemory.h
        src/core/cmemory.c
        src/core/cstring.h
        src/core/cstring.c
        src/core/event.h
        src/core/event.c
        src/containers/darray.h
        src/containers/darray.c
        src/core/input.h
        src/core/input.c
        src/core/clock.h
        src/core/clock.c
        src/renderer/renderer_frontend.h
        src/renderer/renderer_frontend.c
        src/renderer/renderer_backend.h
        src/renderer/renderer_backend.c
        src/renderer/vulkan/vulkan_types.inl
        src/renderer/vulkan/vulkan_backend.h
        src/renderer/vulkan/vulkan_backend.c
        src/renderer/vulkan/vulkan_platform.h
        src/renderer/vulkan/vulkan_device.h
        src/renderer/vulkan/vulkan_device.c
        src/renderer/vulkan/vulkan_swapchain.h
        src/renderer/vulkan/vulkan_swapchain.c
        src/renderer/vulkan/vulkan_image.h
        src/renderer/vulkan/vulkan_image.c
        src/renderer/vulkan/vulkan_renderpass.h
        src/renderer/vulkan/vulkan_renderpass.c
        src/renderer/vulkan/vulkan_command_buffer.h
        src/renderer/vulkan/vulkan_command_buffer.c)

target_include_directories(
        cEngine
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(cEngine PRIVATE
        ${X11_LIBRARIES}
        ${XCB_LIBRARIES}
        vulkan
        X11-xcb)

add_subdirectory(example)
