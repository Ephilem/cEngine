#pragma once

#include "vulkan_types.inl"


void vulkan_image_create(
    vulkan_context* context,
    VkImageType type,
    u32 width,
    u32 height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags memory_flags,
    b32 create_view,
    VkImageAspectFlags view_aspect_flags,
    vulkan_image* image);

void vulkan_image_view_create(
    vulkan_context* context,
    VkFormat format,
    vulkan_image* image,
    VkImageAspectFlags aspect_flags);

/**
 * Transition the image layout from old_layout to new_layout.
 */
void vulkan_image_transition_layout(
    vulkan_context* context,
    vulkan_command_buffer* command_buffer,
    vulkan_image* image,
    VkFormat format,
    VkImageLayout old_layout,
    VkImageLayout new_layout);

/**
 * Copies data in buffer to provided image.
 * @param context Vulkan context
 * @param image Image to copy data to
 * @param buffer Buffer to copy data from
 * @param command_buffer Command buffer to record the copy command to
 */
void vulkan_image_copy_from_buffer(
    vulkan_context* context,
    vulkan_image* image,
    VkBuffer buffer,
    vulkan_command_buffer* command_buffer);


void vulkan_image_destroy(vulkan_context* context, vulkan_image* image);
