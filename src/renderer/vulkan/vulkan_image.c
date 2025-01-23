#include "vulkan_image.h"

#include "vulkan_device.h"

#include "core/cmemory.h"
#include "core/logger.h"

void vulkan_image_create(vulkan_context *context, VkImageType type, u32 width, u32 height, VkFormat format,
    VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_flags, b32 create_view,
    VkImageAspectFlags view_aspect_flags, vulkan_image *image) {

    // copy param
    image->width = width;
    image->height = height;

    VkImageCreateInfo create_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    create_info.imageType = VK_IMAGE_TYPE_2D;
    create_info.extent.width = width;
    create_info.extent.height = height;
    create_info.extent.depth = 1; //
    create_info.mipLevels = 4;
    create_info.arrayLayers = 1;
    create_info.format = format;
    create_info.tiling = tiling;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    create_info.usage = usage;
    create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK(vkCreateImage(context->device.logical, &create_info, context->allocator, &image->handle));

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(context->device.logical, image->handle, &memory_requirements);

    // search for the memory type in the memory properties
    i32 memory_type = context->find_memory_index(memory_requirements.memoryTypeBits, memory_flags);
    if (memory_type == -1) {
        LOG_ERROR("Failed to find a suitable memory type for the image");
    }

    VkMemoryAllocateInfo memory_allocate_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    memory_allocate_info.allocationSize = memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = memory_type;
    VK_CHECK(vkAllocateMemory(context->device.logical, &memory_allocate_info, context->allocator, &image->memory));

    // Bind
    VK_CHECK(vkBindImageMemory(context->device.logical, image->handle, image->memory, 0));

    // Create view
    if (create_view) {
        image->view = 0;
        vulkan_image_view_create(context, format, image, view_aspect_flags);
    }
}

void vulkan_image_view_create(vulkan_context *context, VkFormat format, vulkan_image *image,
    VkImageAspectFlags aspect_flags) {

    VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view_info.image = image->handle;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = aspect_flags;

    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(context->device.logical, &view_info, context->allocator, &image->view));
}

void vulkan_image_destroy(vulkan_context *context, vulkan_image *image) {
    if (image->view) {
        vkDestroyImageView(context->device.logical, image->view, context->allocator);
        image->view = 0;
    }
    if (image->memory) {
        vkFreeMemory(context->device.logical, image->memory, context->allocator);
        image->memory = 0;
    }
    if (image->handle) {
        vkDestroyImage(context->device.logical, image->handle, context->allocator);
        image->handle = 0;
    }
}
