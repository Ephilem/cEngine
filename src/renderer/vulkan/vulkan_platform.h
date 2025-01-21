#pragma once

#include "define.h"

struct platform_state;
struct vulkan_context;

b8 platform_create_vulkan_surface(
    struct platform_state* platform_state,
    struct vulkan_context* context);

/**
 * Appends the names of required externsion for this platform to
 * the extensions list.
 * @param extensions darray of extension names
 */
void platform_get_required_extension_names(const char*** extensions);
