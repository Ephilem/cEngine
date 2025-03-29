cmake_minimum_required(VERSION 3.30)
project(cEngine C)

set(CMAKE_C_STANDARD 11)

# Trouver le compilateur GLSL (glslc de Vulkan)
find_program(GLSL_COMPILER glslc)

if(NOT GLSL_COMPILER)
    message(FATAL_ERROR "glslc non trouvé! Veuillez installer le SDK Vulkan.")
endif()

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
        src/renderer/vulkan/vulkan_command_buffer.c
        src/renderer/vulkan/vulkan_framebuffer.c
        src/renderer/vulkan/vulkan_framebuffer.h
        src/renderer/vulkan/vulkan_fence.c
        src/renderer/vulkan/vulkan_fence.h
        src/renderer/vulkan/vulkan_utils.c
        src/renderer/vulkan/vulkan_utils.h
        src/math/math_types.h
        src/math/cmath.h
        src/math/cmath.c)

target_include_directories(
        cEngine
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(cEngine PRIVATE
        ${X11_LIBRARIES}
        ${XCB_LIBRARIES}
        vulkan
        X11-xcb)

# Fonction pour compiler les shaders
function(compile_shader TARGET SHADER)
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SHADER_OUTPUT "${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.spv")

    # Créer le répertoire de sortie s'il n'existe pas
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

    # Définir la commande de compilation du shader
    add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${GLSL_COMPILER} ${SHADER} -o ${SHADER_OUTPUT}
            DEPENDS ${SHADER}
            COMMENT "Compilation du shader ${SHADER_NAME}"
    )

    # Ajouter le fichier de sortie à la propriété SHADER_OUTPUTS du target
    set_property(
            TARGET ${TARGET}
            APPEND PROPERTY SHADER_OUTPUTS ${SHADER_OUTPUT}
    )
endfunction()

# Rechercher tous les fichiers de shaders
file(GLOB SHADER_SOURCES "src/shaders/*.vert" "src/shaders/*.frag" "src/shaders/*.comp")

# Compiler chaque shader trouvé
foreach(SHADER ${SHADER_SOURCES})
    compile_shader(cEngine ${SHADER})
endforeach()

# Créer une cible personnalisée pour les shaders
get_target_property(SHADER_OUTPUTS cEngine SHADER_OUTPUTS)
if(SHADER_OUTPUTS)
    add_custom_target(Shaders DEPENDS ${SHADER_OUTPUTS})
    add_dependencies(cEngine Shaders)
else()
    # Si aucun shader n'est présent, créer une cible vide
    add_custom_target(Shaders)
    add_dependencies(cEngine Shaders)
endif()

add_subdirectory(example)
