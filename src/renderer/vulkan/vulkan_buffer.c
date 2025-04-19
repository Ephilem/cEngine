#include "vulkan_buffer.h"

#include "vulkan_command_buffer.h"
#include "vulkan_utils.h"
#include "core/cmemory.h"
#include "core/logger.h"

b8 vulkan_buffer_create(vulkan_context *context, u64 size, VkBufferUsageFlagBits usage, u32 memory_property_flags,
                        b8 bind_on_create, vulkan_buffer *out_buffer) {

    czero_memory(out_buffer, sizeof(vulkan_buffer));
    out_buffer->total_size = size;
    out_buffer->usage = usage;
    out_buffer->mmeory_property_flags = memory_property_flags;

    VkBufferCreateInfo buffer_create_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_create_info.size = size;
    buffer_create_info.usage = usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // only used in one queue

    VK_CHECK(vkCreateBuffer(context->device.logical, &buffer_create_info, context->allocator, &out_buffer->handle));

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(context->device.logical, out_buffer->handle, &memory_requirements);
    out_buffer->memory_index = context->find_memory_index(memory_requirements.memoryTypeBits, memory_property_flags);
    if (out_buffer->memory_index == -1) {
        LOG_ERROR("Unable to create vulkan buffer because the required memory type index was not found...");
        return false;
    }

    // allcoate memory info
    VkMemoryAllocateInfo allocate_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = (u32)out_buffer->memory_index;

    VkResult result = vkAllocateMemory(
        context->device.logical,
        &allocate_info,
        context->allocator,
        &out_buffer->memory);

    if (result != VK_SUCCESS) {
        LOG_ERROR("Unable to allocate memory for vulkan buffer: %s", vulkan_result_string(result, true));
        return false;
    }

    if (bind_on_create) {
        vulkan_buffer_bind(context, out_buffer, 0);
    }

    return true;
}

void vulkan_buffer_destroy(vulkan_context *context, vulkan_buffer *buffer) {
    if (buffer->memory) {
        vkFreeMemory(context->device.logical, buffer->memory, context->allocator);
        buffer->memory = 0;
    }
    if (buffer->handle) {
        vkDestroyBuffer(context->device.logical, buffer->handle, context->allocator);
        buffer->handle = 0;
    }

    buffer->total_size = 0;
    buffer->usage = 0;
    buffer->is_locked = false;
}

b8 vulkan_buffer_resize(vulkan_context *context, u64 new_size, vulkan_buffer *buffer, VkQueue queue,
    VkCommandPool pool) {
    // create the new buffer
    VkBufferCreateInfo buffer_create_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_create_info.size = new_size;
    buffer_create_info.usage = buffer->usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // only used in one queue

    VkBuffer new_buffer;
    VK_CHECK(vkCreateBuffer(context->device.logical, &buffer_create_info, context->allocator, &new_buffer));

    // gather memory requirements
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(context->device.logical, new_buffer, &memory_requirements);

    // allocate memory for the new buffer
    VkMemoryAllocateInfo allocate_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = (u32)buffer->memory_index;

    VkDeviceMemory new_memory;
    VkResult result = vkAllocateMemory(
        context->device.logical,
        &allocate_info,
        context->allocator,
        &new_memory);

    if (result != VK_SUCCESS) {
        LOG_ERROR("Unable to allocate memory for vulkan buffer: %s", vulkan_result_string(result, true));
        return false;
    }

    // bind the new buffer memory
    VK_CHECK(vkBindBufferMemory(context->device.logical, new_buffer, new_memory, 0));

    // copy the old buffer to the new one
    vulkan_buffer_copy_to(context, pool, 0, queue, buffer->handle, 0, new_buffer, 0, buffer->total_size);

    vkDeviceWaitIdle(context->device.logical);

    // destroy the old buffer
    if (buffer->handle) {
        vkDestroyBuffer(context->device.logical, buffer->handle, context->allocator);
        buffer->handle = 0;
    }
    if (buffer->memory) {
        vkFreeMemory(context->device.logical, buffer->memory, context->allocator);
        buffer->memory = 0;
    }

    buffer->total_size = new_size;
    buffer->handle = new_buffer;
    buffer->memory = new_memory;

    return true;
}

void vulkan_buffer_bind(vulkan_context *context, vulkan_buffer *buffer, u64 offset) {
    VK_CHECK(vkBindBufferMemory(
        context->device.logical,
        buffer->handle,
        buffer->memory,
        offset));
}

void * vulkan_buffer_lock_memory(vulkan_context *context, vulkan_buffer *buffer, u64 offset, u64 size, u32 flags) {
    void* data;
    VK_CHECK(vkMapMemory(context->device.logical, buffer->memory, offset, size, flags, &data));
    return data;
}

void vulkan_buffer_unlock_mecmory(vulkan_context *context, vulkan_buffer *buffer) {
    vkUnmapMemory(context->device.logical, buffer->memory);
}

void vulkan_buffer_load_data(vulkan_context *context, vulkan_buffer *buffer, u64 offset, u64 size, u32 flags,
    const void *data) {

    void* data_ptr;
    VK_CHECK(vkMapMemory(context->device.logical, buffer->memory, offset, size, flags, &data_ptr));
    ccopy_memory(data_ptr, data, size);
    vkUnmapMemory(context->device.logical, buffer->memory);
}

void vulkan_buffer_copy_to(vulkan_context *context, VkCommandPool pool, VkFence fence, VkQueue queue, VkBuffer source,
    u64 source_offset, VkBuffer dest, u64 dest_offset, u64 size) {

    vkQueueWaitIdle(queue);

    // creating one time use command buffer
    vulkan_command_buffer command_buffer;
    vulkan_command_buffer_allocate_and_begin_single_use(context, pool, &command_buffer);

    VkBufferCopy copy_region;
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = size;

    vkCmdCopyBuffer(command_buffer.handle, source, dest, 1, &copy_region);

    vulkan_command_buffer_end_single_use(context, pool, &command_buffer, queue);
}
