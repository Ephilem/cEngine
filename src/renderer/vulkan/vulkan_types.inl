#pragma once

#include "define.h"

#include <vulkan/vulkan.h>

typedef struct vulkan_context {
    VkInstance instance;
    VkAllocationCallbacks* allocator;

    VkPhysicalDevice physical_device;
    VkDevice device;
} vulkan_context;