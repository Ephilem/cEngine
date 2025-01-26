#include "vulkan_swapchain.h"

#include "vulkan_device.h"
#include "vulkan_framebuffer.h"
#include "vulkan_image.h"
#include "vulkan_renderpass.h"
#include "core/cmemory.h"
#include "core/logger.h"

void create(vulkan_context* context, u32 width, u32 height, vulkan_swapchain* swapchain);
void destroy(vulkan_context* context, vulkan_swapchain* swapchain);

void vulkan_swapchain_create(vulkan_context *context, u32 width, u32 height, vulkan_swapchain *out_swapchain) {
    create(context, width, height, out_swapchain);
}

void vulkan_swapchain_recreate(vulkan_context *context, u32 width, u32 height, vulkan_swapchain *swapchain) {
    // Destroy the old swapchain
    destroy(context, swapchain);

    // Recreate the swapchain
    create(context, width, height, swapchain);

    // Destroy the old render pass
    vulkan_renderpass_destroy(context, &context->main_renderpass);

    // Recreate the render pass with the new swapchain's format
    vulkan_renderpass_create(
        context,
        &context->main_renderpass,
        0, 0,
        width, height,
        0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 0);

    // Regenerate framebuffers with the new render pass and swapchain image views
    vulkan_swapchain_regenerate_framebuffers(context, swapchain, &context->main_renderpass);
}

void vulkan_swapchain_destroy(vulkan_context *context, vulkan_swapchain *swapchain) {
    destroy(context, swapchain);
}

b8 vulkan_swapchain_acquire_next_image_index(
    vulkan_context* context,
    vulkan_swapchain* swapchain,
    u64 timeout_ns,
    VkSemaphore image_available_semaphore,
    VkFence fence,
    u32* out_image_index) {
    VkResult result = vkAcquireNextImageKHR(
        context->device.logical,
        swapchain->handle,
        timeout_ns,
        image_available_semaphore,
        fence,
        out_image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // Trigger swapchain recreation, then boot out of the render loop.
        LOG_DEBUG("vulkan_swapchain_acquire_next_image_index: Swapchain out of date, recreating...");
        vulkan_swapchain_recreate(context, context->framebuffer_width, context->framebuffer_height, swapchain);
        return FALSE;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        LOG_FATAL("Failed to acquire swapchain image!");
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
    present_info.pSwapchains = &swapchain->handle;
    present_info.pImageIndices = &present_image_index;
    present_info.pResults = 0;

    VkResult result = vkQueuePresentKHR(present_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        LOG_DEBUG("vulkan_swapchain_present: Swapchain out of date or suboptimal, recreating...");
        vulkan_swapchain_recreate(context, context->framebuffer_width, context->framebuffer_height, swapchain);
    } else if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to present the swapchain image");
    }

    // switch to the next frame
    context->current_frame = (context->current_frame + 1) % swapchain->max_frames_in_flight;
}

void vulkan_swapchain_regenerate_framebuffers(vulkan_context *context, vulkan_swapchain *swapchain,
    vulkan_renderpass *renderpass) {

    for (u32 i = 0; i < swapchain->image_count; ++i) {
        u32 attachment_count = 2;
        VkImageView attachments[] = {
            swapchain->views[i],
            swapchain->depth_attachment.view
        };

        vulkan_framebuffer_create(
            context,
            renderpass,
            swapchain->extent.width, // Use swapchain's extent, not context->framebuffer_width
            swapchain->extent.height,
            attachment_count,
            attachments,
            &swapchain->framebuffers[i]);

        LOG_TRACE("Regenerating framebuffer %d (frambuffer pointer %p | image view pointer %p)", i, &swapchain->framebuffers[i], swapchain->views[i]);
    }
}

void create(vulkan_context *context, u32 width, u32 height, vulkan_swapchain *swapchain) {
    LOG_DEBUG("Starting swapchain creation");

    VkExtent2D swapchain_extent = {width, height};
    swapchain->extent = swapchain_extent;
    swapchain->max_frames_in_flight = 2;

    // Choose a swap surface format.
    b8 found = FALSE;
    for (u32 i = 0; i < context->device.swapchain_support_info.format_count; ++i) {
        VkSurfaceFormatKHR format = context->device.swapchain_support_info.formats[i];
        // Preferred formats
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            swapchain->format = format;
            found = TRUE;
            break;
        }
    }

    if (!found) {
        swapchain->format = context->device.swapchain_support_info.formats[0];
    }

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (u32 i = 0; i < context->device.swapchain_support_info.present_mode_count; ++i) {
        VkPresentModeKHR mode = context->device.swapchain_support_info.present_modes[i];
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = mode;
            break;
        }
    }

    // Requery swapchain support.
    vulkan_device_query_swapchain_support(
        context->device.physical,
        context->surface,
        &context->device.swapchain_support_info);

    // Swapchain extent
    if (context->device.swapchain_support_info.capabilities.currentExtent.width != UINT32_MAX) {
        swapchain_extent = context->device.swapchain_support_info.capabilities.currentExtent;
    }

    // Clamp to the value allowed by the GPU.
    VkExtent2D min = context->device.swapchain_support_info.capabilities.minImageExtent;
    VkExtent2D max = context->device.swapchain_support_info.capabilities.maxImageExtent;
    swapchain_extent.width = cCLAMP(swapchain_extent.width, min.width, max.width);
    swapchain_extent.height = cCLAMP(swapchain_extent.height, min.height, max.height);

    u32 image_count = context->device.swapchain_support_info.capabilities.minImageCount + 1;
    if (context->device.swapchain_support_info.capabilities.maxImageCount > 0 && image_count > context->device.swapchain_support_info.capabilities.maxImageCount) {
        image_count = context->device.swapchain_support_info.capabilities.maxImageCount;
    }

    // Swapchain create info
    VkSwapchainCreateInfoKHR swapchain_create_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchain_create_info.surface = context->surface;
    swapchain_create_info.minImageCount = image_count;
    swapchain_create_info.imageFormat = swapchain->format.format;
    swapchain_create_info.imageColorSpace = swapchain->format.colorSpace;
    swapchain_create_info.imageExtent = swapchain_extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Setup the queue family indices
    if (context->device.graphics_queue_index != context->device.present_queue_index) {
        u32 queueFamilyIndices[] = {
            (u32)context->device.graphics_queue_index,
            (u32)context->device.present_queue_index};
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices = 0;
    }

    swapchain_create_info.preTransform = context->device.swapchain_support_info.capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = present_mode;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = 0;

    VK_CHECK(vkCreateSwapchainKHR(context->device.logical, &swapchain_create_info, context->allocator, &swapchain->handle));

    // Start with a zero frame index.
    context->current_frame = 0;

    // Images
    swapchain->image_count = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(context->device.logical, swapchain->handle, &swapchain->image_count, 0));
    if (!swapchain->images) {
        swapchain->images = (VkImage*)callocate(sizeof(VkImage) * swapchain->image_count, MEMORY_TAG_RENDERER);
    }
    if (!swapchain->views) {
        swapchain->views = (VkImageView*)callocate(sizeof(VkImageView) * swapchain->image_count, MEMORY_TAG_RENDERER);
    }
    VK_CHECK(vkGetSwapchainImagesKHR(context->device.logical, swapchain->handle, &swapchain->image_count, swapchain->images));

    // Views
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

        VK_CHECK(vkCreateImageView(context->device.logical, &view_info, context->allocator, &swapchain->views[i]));

        LOG_TRACE("Creating image view %d (pointer %p)", i, swapchain->views[i]);
    }

    // Depth resources
    if (!vulkan_device_detect_depth_format(&context->device)) {
        context->device.depth_format = VK_FORMAT_UNDEFINED;
        LOG_FATAL("Failed to find a supported format!");
    }

    // Create depth image and its view.
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


    LOG_DEBUG("Swapchain created successfully");
}

void destroy(vulkan_context *context, vulkan_swapchain *swapchain) {
    vkDeviceWaitIdle(context->device.logical);
    LOG_DEBUG("Destroying swapchain");
    vulkan_image_destroy(context, &swapchain->depth_attachment);

    for (u32 i = 0; i < swapchain->image_count; ++i) {
        vkDestroyImageView(context->device.logical, swapchain->views[i], context->allocator);
    }

    vkDestroySwapchainKHR(context->device.logical, swapchain->handle, context->allocator);
}
