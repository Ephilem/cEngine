#pragma once

#include "vulkan_types.inl"

void vulkan_swapchain_create(
    vulkan_context* context,
    u32 width,
    u32 height,
    vulkan_swapchain* out_swapchain);

// Recreate the swapchain when for example the window is resized
void vulkan_swapchain_recreate(
    vulkan_context* context,
    u32 width,
    u32 height,
    vulkan_swapchain* swapchain);

void vulkan_swapchain_destroy(
    vulkan_context* context,
    vulkan_swapchain* swapchain);

/**
 * Give the index of the next image to render after the semaphore is signaled
 * @param context vulkan context
 * @param swapchain vulkan swapchain
 * @param timeout_ns timeout in nanoseconds
 * @param image_available_semaphore semaphore to signal when the image is available
 * @param fence fence to signal when the image is available
 * @param out_image_index index of the image to render
 * @return TRUE if the image is acquired, FALSE otherwise
 */
b8 vulkan_swapchain_acquire_next_image_index(
    vulkan_context* context,
    vulkan_swapchain* swapchain,
    u64 timeout_ns,
    VkSemaphore image_available_semaphore,
    VkFence fence,
    u32* out_image_index);

void vulkan_swapchain_present(
    vulkan_context* context,
    vulkan_swapchain* swapchain,
    VkQueue graphics_queue,
    VkQueue present_queue,
    VkSemaphore render_complete_semaphore,
    u32 present_image_index);

void vulkan_swapchain_regenerate_framebuffers(
    vulkan_context* context,
    vulkan_swapchain* swapchain,
    vulkan_renderpass* renderpass);
