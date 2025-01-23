#pragma once

#include "vulkan_types.inl"

void vulkan_command_buffer_allocate_from_pool(
    vulkan_context* context,
    VkCommandPool pool,
    b8 is_primary,
    vulkan_command_buffer* out_command_buffer);

void vulkan_command_buffer_free_from_pool(
    vulkan_context* context,
    VkCommandPool pool,
    vulkan_command_buffer* command_buffer);


void vulkan_command_buffer_begin_recording(
    vulkan_command_buffer* command_buffer,
    b8 is_single_use,
    b8 is_renderpass_continue,
    b8 is_simultaneous_use);

void vulkan_command_buffer_end_recording(vulkan_command_buffer* command_buffer);

void vulkan_command_buffer_update_submitted(vulkan_command_buffer* command_buffer);

void vulkan_command_buffer_reset(vulkan_command_buffer* command_buffer);

/**
 * Allocate from a pool and begin recording a single use command buffer
 * @param context the vulkan context
 * @param pool the commands pool
 * @param out_command_buffer the command buffer to allocate and begin recording
 */
void vulkan_command_buffer_allocate_and_begin_single_use(
    vulkan_context* context,
    VkCommandPool pool,
    vulkan_command_buffer* out_command_buffer);

/**
 * End recording, submits to and wits for queue operation and frees the provided command buffer
 */
void vulkan_command_buffer_end_single_use(
    vulkan_context* context,
    VkCommandPool pool,
    vulkan_command_buffer* command_buffer,
    VkQueue queue);



