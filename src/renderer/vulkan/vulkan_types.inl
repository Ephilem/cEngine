#pragma once

#include "define.h"

#include <vulkan/vulkan.h>
#include "core/asserts.h"

#define VK_CHECK(expr) \
    { \
        cASSERT(expr == VK_SUCCESS); \
    }

typedef struct vulkan_swapchain_support_info {
    VkSurfaceCapabilitiesKHR capabilities;
    u32 format_count;
    VkSurfaceFormatKHR* formats;
    u32 present_mode_count;
    VkPresentModeKHR* present_modes;
} vulkan_swapchain_support_info;

typedef struct vulkan_device {
    VkPhysicalDevice physical;
    VkDevice logical;
    vulkan_swapchain_support_info swapchain_support_info;
    i32 graphics_queue_index;
    i32 present_queue_index;
    i32 compute_queue_index;
    i32 transfer_queue_index;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;
} vulkan_device;

typedef struct vulkan_context {
    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;

#if _DEBUG
    VkDebugUtilsMessengerEXT debug_messenger;
#endif
    vulkan_device device;
} vulkan_context;