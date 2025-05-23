#include "vulkan_command_buffer.h"

#include "vulkan_utils.h"
#include "core/cmemory.h"
#include "core/logger.h"

void vulkan_command_buffer_allocate_from_pool(vulkan_context *context, VkCommandPool pool, b8 is_primary,
                                              vulkan_command_buffer *out_command_buffer) {

    czero_memory(out_command_buffer, sizeof(out_command_buffer));

    VkCommandBufferAllocateInfo allocate_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocate_info.commandPool = pool;
    allocate_info.level = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocate_info.commandBufferCount = 1;
    allocate_info.pNext = 0;

    out_command_buffer->state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;
    VK_CHECK(vkAllocateCommandBuffers(context->device.logical, &allocate_info, &out_command_buffer->handle));
    out_command_buffer->state = COMMAND_BUFFER_STATE_READY;
}

void vulkan_command_buffer_free_from_pool(vulkan_context *context, VkCommandPool pool, vulkan_command_buffer *command_buffer) {
    vkFreeCommandBuffers(context->device.logical, pool, 1, &command_buffer->handle);
    command_buffer->handle = 0;
    command_buffer->state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;
}

void vulkan_command_buffer_begin_recording(vulkan_command_buffer *command_buffer, b8 is_single_use,
    b8 is_renderpass_continue, b8 is_simultaneous_use) {
    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = 0;

    if (is_single_use) {
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    }
    if (is_renderpass_continue) {
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    }
    if (is_simultaneous_use) {
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    }

    VK_CHECK(vkBeginCommandBuffer(command_buffer->handle, &begin_info));
    command_buffer->state = COMMAND_BUFFER_STATE_RECORDING;
}

void vulkan_command_buffer_end_recording(vulkan_command_buffer *command_buffer) {
    VK_CHECK(vkEndCommandBuffer(command_buffer->handle));
    command_buffer->state = COMMAND_BUFFER_STATE_RECORDING_ENDED;
}

void vulkan_command_buffer_update_submitted(vulkan_command_buffer *command_buffer) {
    command_buffer->state = COMMAND_BUFFER_STATE_SUBMITTED;
}

void vulkan_command_buffer_reset(vulkan_command_buffer *command_buffer) {
    command_buffer->state = COMMAND_BUFFER_STATE_READY;
}

void vulkan_command_buffer_allocate_and_begin_single_use(vulkan_context *context, VkCommandPool pool,
    vulkan_command_buffer *out_command_buffer) {
    vulkan_command_buffer_allocate_from_pool(context, pool, true, out_command_buffer);
    vulkan_command_buffer_begin_recording(out_command_buffer, true, false, false);
}

void vulkan_command_buffer_end_single_use(vulkan_context *context, VkCommandPool pool,
    vulkan_command_buffer *command_buffer, VkQueue queue) {

    vulkan_command_buffer_end_recording(command_buffer);

    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer->handle;
    VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, 0));

    // wait for finish
    VK_CHECK(vkQueueWaitIdle(queue));

    // free the command buffer
    vulkan_command_buffer_free_from_pool(context, pool, command_buffer);
}
