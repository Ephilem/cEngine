#include "vulkan_swapchain.h"

#include "vulkan_device.h"
#include "vulkan_image.h"
#include "core/cmemory.h"
#include "core/logger.h"

void create(vulkan_context* context, u32 width, u32 height, vulkan_swapchain* swapchain);
void destroy(vulkan_context* context, vulkan_swapchain* swapchain);

void vulkan_swapchain_create(vulkan_context *context, u32 width, u32 height, vulkan_swapchain *swapchain) {
    create(context, width, height, swapchain);
}

void vulkan_swapchain_recreate(vulkan_context *context, u32 width, u32 height, vulkan_swapchain *swapchain) {
    destroy(context, swapchain);
    create(context, width, height, swapchain);
}

void vulkan_swapchain_destroy(vulkan_context *context, vulkan_swapchain *swapchain) {
    destroy(context, swapchain);
}

b8 vulkan_swapchain_acquire_next_image(vulkan_context *context, vulkan_swapchain *swapchain, u64 timeout_ns,
    VkSemaphore image_available_semaphore, VkFence fence, u32 *out_image_index) {

    // return our image index
    VkResult result = vkAcquireNextImageKHR(context->device.logical, swapchain->swapchain, timeout_ns, image_available_semaphore, fence, out_image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // mean that the swapchain need a recreation
        vulkan_swapchain_recreate(context, context->framebuffer_width, context->framebuffer_height, swapchain);
        return FALSE;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        LOG_ERROR("Failed to acquire a swapchain image");
        return FALSE;
    }

    return TRUE;
}

void vulkan_swapchain_present(vulkan_context *context, vulkan_swapchain *swapchain, VkQueue graphics_queue,
    VkQueue present_queue, VkSemaphore render_complete_semaphore, u32 present_image_index) {

    VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &render_complete_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain->swapchain;
    present_info.pImageIndices = &present_image_index;
    present_info.pResults = 0;

    VkResult result = vkQueuePresentKHR(present_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        vulkan_swapchain_recreate(context, context->framebuffer_width, context->framebuffer_height, swapchain);
    } else if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to present the swapchain image");
    }
}

void create(vulkan_context *context, u32 width, u32 height, vulkan_swapchain *swapchain) {
    VkExtent2D swapchain_extent = {width, height}; // size
    swapchain->max_frame_in_flight = 2;

    // choose the format of the surface
    b8 found = FALSE;
    for (u32 i = 0; i < context->device.swapchain_support_info.format_count; ++i) {
        VkSurfaceFormatKHR format = context->device.swapchain_support_info.formats[i];
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&  format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            swapchain->format = format;
            found = TRUE;
            break;
        }
    }

    if (!found) {
        swapchain->format = context->device.swapchain_support_info.formats[0];
    }

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (u32 i = 0; i < context->device.swapchain_support_info.present_modes[i]; ++i) {
        VkPresentModeKHR mode = context->device.swapchain_support_info.present_modes[i];
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = mode;
            break;
        }
    }

    vulkan_device_query_swapchain_support(
        context->device.physical,
        context->surface,
        &context->device.swapchain_support_info);

    if (context->device.swapchain_support_info.capabilities.currentExtent.width != UINT32_MAX) {
        swapchain_extent = context->device.swapchain_support_info.capabilities.currentExtent;
    }

    // clamp to the value allowed by the GPU
    VkExtent2D min = context->device.swapchain_support_info.capabilities.minImageExtent;
    VkExtent2D max = context->device.swapchain_support_info.capabilities.maxImageExtent;
    swapchain_extent.width = cCLAMP(swapchain_extent.width, min.width, max.width);
    swapchain_extent.height = cCLAMP(swapchain_extent.height, min.height, max.height);

    u32 image_count = context->device.swapchain_support_info.capabilities.minImageCount + 1;
    if (context->device.swapchain_support_info.capabilities.maxImageCount > 0 && image_count > context->device.swapchain_support_info.capabilities.maxImageCount) {
        image_count = context->device.swapchain_support_info.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    create_info.surface = context->surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = swapchain->format.format;
    create_info.imageColorSpace = swapchain->format.colorSpace;
    create_info.imageExtent = swapchain_extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // make use of different queues if possible
    if (context->device.graphics_queue_index != context->device.present_queue_index) {
        u32 queueFamilyIndices[] = {
            (u32)context->device.graphics_queue_index,
            (u32)context->device.present_queue_index};
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = 0;
    }

    create_info.preTransform = context->device.swapchain_support_info.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = 0;

    VK_CHECK(vkCreateSwapchainKHR(context->device.logical, &create_info, context->allocator, &swapchain->swapchain));

    context->current_frame = 0;

    swapchain->image_count = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(context->device.logical, swapchain->swapchain, &swapchain->image_count, 0));
    if (!swapchain->images) {
        swapchain->images = (VkImage*)callocate(sizeof(VkImage) * swapchain->image_count, MEMORY_TAG_RENDERER);
    }
    if (!swapchain->image_views) {
        swapchain->image_views = (VkImageView*)callocate(sizeof(VkImageView) * swapchain->image_count, MEMORY_TAG_RENDERER);
    }
    VK_CHECK(vkGetSwapchainImagesKHR(context->device.logical, swapchain->swapchain, &swapchain->image_count, swapchain->images));

    // generate views
    for (u32 i = 0; i < swapchain->image_count; ++i) {
        VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view_info.image = swapchain->images[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = swapchain->format.format;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(context->device.logical, &view_info, context->allocator, &swapchain->image_views[i]));
    }

    if (!vulkan_device_detect_depth_format(&context->device)) {
        context->device.depth_format = VK_FORMAT_UNDEFINED;
        LOG_FATAL("No depth format!!!!");
    }

    vulkan_image_create(
        context,
        VK_IMAGE_TYPE_2D,
        swapchain_extent.width,
        swapchain_extent.height,
        context->device.depth_format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        TRUE,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        &swapchain->depth_attachment);

    LOG_TRACE("Swapchain created successfully");
}

void destroy(vulkan_context *context, vulkan_swapchain *swapchain) {
    vulkan_image_destroy(context, &swapchain->depth_attachment);

    for (u32 i = 0; i < swapchain->image_count; ++i) {
        vkDestroyImageView(context->device.logical, swapchain->image_views[i], context->allocator);
    }

    vkDestroySwapchainKHR(context->device.logical, swapchain->swapchain, context->allocator);
}
