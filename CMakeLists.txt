cmake_minimum_required(VERSION 3.30)
project(cEngine C)

set(CMAKE_C_STANDARD 11)

# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=vla") # force C11 and prevent using VLA

find_program(GLSL_COMPILER glslc)

if(NOT GLSL_COMPILER)
    message(FATAL_ERROR "glslc not found!")
endif()

set(ASSETS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
set(ASSETS_OUTPUT_DIR "${CMAKE_BINARY_DIR}/assets")

file(MAKE_DIRECTORY ${ASSETS_OUTPUT_DIR})
file(MAKE_DIRECTORY ${ASSETS_OUTPUT_DIR}/shaders)
file(MAKE_DIRECTORY ${ASSETS_OUTPUT_DIR}/textures)

find_package(Vulkan REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(X11 REQUIRED)
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
        src/renderer/vulkan/vulkan_command_buffer.c
        src/renderer/vulkan/vulkan_framebuffer.c
        src/renderer/vulkan/vulkan_framebuffer.h
        src/renderer/vulkan/vulkan_fence.c
        src/renderer/vulkan/vulkan_fence.h
        src/renderer/vulkan/vulkan_utils.c
        src/renderer/vulkan/vulkan_utils.h
        src/math/math_types.h
        src/math/cmath.h
        src/math/cmath.c
        src/memory/linear_allocator.h
        src/memory/linear_allocator.c
        src/renderer/vulkan/shaders/vulkan_material_shader.h
        src/renderer/vulkan/shaders/vulkan_material_shader.c
        src/renderer/vulkan/vulkan_shader_utils.c
        src/renderer/vulkan/vulkan_shader_utils.h
        src/platform/filesystem.c
        src/platform/filesystem.h
        src/renderer/vulkan/vulkan_pipeline.c
        src/renderer/vulkan/vulkan_pipeline.h
        src/renderer/vulkan/vulkan_buffer.c
        src/renderer/vulkan/vulkan_buffer.h
        src/resources/resource_types.h
        src/containers/hashtable.c
        src/containers/hashtable.h
        src/systems/texture_system.c
        src/systems/texture_system.h
        src/systems/material_system.c
        src/systems/material_system.h
        src/systems/geometry_system.c
        src/systems/geometry_system.h
        src/systems/resource_system.c
        src/systems/resource_system.h
        src/resources/loaders/image_loader.c
        src/resources/loaders/image_loader.h
        src/resources/loaders/material_loader.c
        src/resources/loaders/material_loader.h
        src/resources/loaders/binary_loader.c
        src/resources/loaders/binary_loader.h
        src/resources/loaders/text_loader.c
        src/resources/loaders/text_loader.h)

target_include_directories(
        cEngine
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(cEngine PRIVATE
        ${PLATFORM_LIBS}
        vulkan
        ${X11_LIBRARIES}
        ${XCB_LIBRARIES}
        X11-xcb
        m)

function(compile_shader TARGET SHADER)
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SHADER_OUTPUT "${ASSETS_OUTPUT_DIR}/shaders/${SHADER_NAME}.spv")

    add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${GLSL_COMPILER} ${SHADER} -o ${SHADER_OUTPUT}
            DEPENDS ${SHADER}
            COMMENT "Compilation of ${SHADER_NAME}"
    )

    set_property(
            TARGET ${TARGET}
            APPEND PROPERTY SHADER_OUTPUTS ${SHADER_OUTPUT}
    )
endfunction()

# Make sure to reconfigure when shader files change
file(GLOB_RECURSE SHADER_SOURCES_FOR_RECONFIGURE "${ASSETS_SOURCE_DIR}/shaders/*.vert" "${ASSETS_SOURCE_DIR}/shaders/*.frag" "${ASSETS_SOURCE_DIR}/shaders/*.comp")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${SHADER_SOURCES_FOR_RECONFIGURE})

file(GLOB SHADER_SOURCES "${ASSETS_SOURCE_DIR}/shaders/*.vert" "${ASSETS_SOURCE_DIR}/shaders/*.frag" "${ASSETS_SOURCE_DIR}/shaders/*.comp")

foreach(SHADER ${SHADER_SOURCES})
    compile_shader(cEngine ${SHADER})
endforeach()

get_target_property(SHADER_OUTPUTS cEngine SHADER_OUTPUTS)
if(SHADER_OUTPUTS)
    add_custom_target(Shaders DEPENDS ${SHADER_OUTPUTS})
    add_dependencies(cEngine Shaders)
else()
    add_custom_target(Shaders)
    add_dependencies(cEngine Shaders)
endif()

add_custom_target(CopyAssets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${ASSETS_SOURCE_DIR}
    ${ASSETS_OUTPUT_DIR}
    COMMENT "Copying textures to build assets directory"
)

add_dependencies(cEngine CopyAssets)

add_subdirectory(example)
add_subdirectory(tests)

add_custom_command(
    TARGET Shaders POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${ASSETS_OUTPUT_DIR}/shaders
    ${ASSETS_SOURCE_DIR}/shaders
    COMMENT "Updating compiled shaders in source assets directory"
)