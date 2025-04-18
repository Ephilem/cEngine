#include "vulkan_backend.h"

#include "vulkan_device.h"
#include "vulkan_platform.h"
#include "vulkan_swapchain.h"
#include "vulkan_renderpass.h"
#include "vulkan_command_buffer.h"
#include "vulkan_framebuffer.h"
#include "vulkan_fence.h"
#include "vulkan_utils.h"
#include "vulkan_types.inl"

#include "core/logger.h"
#include "core/cstring.h"
#include "core/cmemory.h"

#include "containers/darray.h"
#include "core/application.h"

#include "platform/platform.h"

// static vulkan context
static vulkan_context context;
static u32 cached_framebuffer_width = 0;
static u32 cached_framebuffer_height = 0;

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data);

i32 find_memory_index(u32 type_filter, u32 property_flags);

void create_command_buffers(renderer_backend* backend);
void regenerate_framebuffers(renderer_backend* backend, vulkan_swapchain* swapchain, vulkan_renderpass* renderpass);
b8 recreate_swapchain(renderer_backend* backend);

b8 vulkan_renderer_backend_initialize(struct renderer_backend *backend, const char *application_name, struct platform_state *platform_state) {
    // TODO use platform allocator with the tag MEMORY_TAG_VULKAN
    context.allocator = 0;
    context.find_memory_index = find_memory_index;

    application_get_framebuffer_size(&context.framebuffer_width, &context.framebuffer_height);
    context.framebuffer_width = (cached_framebuffer_width != 0) ? cached_framebuffer_width : 800;
    context.framebuffer_height = (cached_framebuffer_height != 0) ? cached_framebuffer_height : 600;
    cached_framebuffer_width = 0;
    cached_framebuffer_height = 0;

    VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.apiVersion = VK_API_VERSION_1_3;
    app_info.pApplicationName = application_name;
    app_info.pEngineName = "cEngine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    VkInstanceCreateInfo create_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    create_info.pApplicationInfo = &app_info;

    // obtain a list of required extensions
    const char** required_extensions = darray_create(const char*);
    darray_push(required_extensions, &VK_KHR_SURFACE_EXTENSION_NAME);

    platform_get_required_extension_names(&required_extensions);

#if defined(_DEBUG)
    darray_push(required_extensions, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    create_info.enabledExtensionCount = darray_length(required_extensions);
    create_info.ppEnabledExtensionNames = required_extensions;

    LOG_TRACE("Needed extensions:");
    for (int i = 0; i < create_info.enabledExtensionCount; ++i) {
        LOG_TRACE(" - %s", create_info.ppEnabledExtensionNames[i]);
    }

    const char** required_validation_layer_names = 0;
    u32 required_validation_layer_count = 0;

#if _DEBUG
    LOG_DEBUG("Loading vulkan validation layers for debugging");

    required_validation_layer_names = darray_create(const char*);
    darray_push(required_validation_layer_names, &"VK_LAYER_KHRONOS_validation");
    required_validation_layer_count = darray_length(required_validation_layer_names);

    // fetch required validation layers
    u32 available_layer_count = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, 0));
    VkLayerProperties* available_layers = darray_reserve(VkLayerProperties, available_layer_count);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers));

    for (u32 i = 0; i < required_validation_layer_count; ++i) {
        LOG_TRACE("Searching for layer : %s", required_validation_layer_names[i]);
        b8 found = false;
        for (u32 j = 0; j < available_layer_count; ++j) {
            if (string_equals(required_validation_layer_names[i], available_layers[j].layerName) == 0) {
                found = true;
                LOG_TRACE("Found!");
                break;
            }
        }

        if (!found) {
            LOG_FATAL("The required layer %s is not available", required_validation_layer_names[i]);
            return false;
        }
    }
#endif

    create_info.enabledLayerCount = required_validation_layer_count;
    create_info.ppEnabledLayerNames = required_validation_layer_names;

    LOG_DEBUG("Enabled layers:");
    for (u32 i = 0; i < create_info.enabledLayerCount; ++i) {
        LOG_DEBUG(" - %s", create_info.ppEnabledLayerNames[i]);
    }

    VK_CHECK(vkCreateInstance(&create_info, NULL, &context.instance));

#if _DEBUG
    LOG_DEBUG("Loading the debugger");
    u32 log_severity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debug_create_info.messageSeverity = log_severity;
    debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_create_info.pfnUserCallback = vk_debug_callback;

    // search for the function to create the debugger
    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkCreateDebugUtilsMessengerEXT");
    cASSERT_MSG(func, "Failed to found the function to load the debugger");
    VK_CHECK(func(context.instance, &debug_create_info, context.allocator, &context.debug_messenger));
#endif

    // Surface
    if (!platform_create_vulkan_surface(platform_state, &context)) {
        LOG_ERROR("Failed to create the vulkan surface");
        return false;
    }

    if (!vulkan_device_create(&context)) {
        LOG_ERROR("Failed to create the device");
        return false;
    }

    vulkan_swapchain_create(&context, context.framebuffer_width, context.framebuffer_width, &context.swapchain);

    vulkan_renderpass_create(&context, &context.main_renderpass, 0, 0, context.framebuffer_width, context.framebuffer_height, 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 0);

    context.swapchain.framebuffers = darray_reserve(vulkan_framebuffer, context.swapchain.image_count);
    regenerate_framebuffers(backend, &context.swapchain, &context.main_renderpass);

    create_command_buffers(backend);

    // create sync objects
    context.image_available_semaphores = darray_reserve(VkSemaphore, context.swapchain.max_frames_in_flight);
    context.queue_complete_semaphores = darray_reserve(VkSemaphore, context.swapchain.max_frames_in_flight);
    context.in_flight_fences = darray_reserve(vulkan_fence, context.swapchain.max_frames_in_flight);

    for (u8 i = 0; i < context.swapchain.max_frames_in_flight; ++i) {
        VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(context.device.logical, &semaphore_create_info, context.allocator, &context.image_available_semaphores[i]);
        vkCreateSemaphore(context.device.logical, &semaphore_create_info, context.allocator, &context.queue_complete_semaphores[i]);

        vulkan_fence_create(&context, true, &context.in_flight_fences[i]);
    }

    context.images_in_flight = darray_reserve(vulkan_fence, context.swapchain.image_count);
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {
        context.images_in_flight[i] = 0;
    }

    LOG_INFO("Vulkan renderer backend initialized");
    return true;
}

void vulkan_renderer_backend_shutdown(struct renderer_backend *backend) {
    vkDeviceWaitIdle(context.device.logical);

    // Sync objects
    for (u8 i = 0; i < context.swapchain.max_frames_in_flight; ++i) {
        if (context.image_available_semaphores[i]) {
            vkDestroySemaphore(
                context.device.logical,
                context.image_available_semaphores[i],
                context.allocator);
            context.image_available_semaphores[i] = 0;
        }
        if (context.queue_complete_semaphores[i]) {
            vkDestroySemaphore(
                context.device.logical,
                context.queue_complete_semaphores[i],
                context.allocator);
            context.queue_complete_semaphores[i] = 0;
        }
        vulkan_fence_destroy(&context, &context.in_flight_fences[i]);
    }
    darray_destroy(context.image_available_semaphores);
    context.image_available_semaphores = 0;

    darray_destroy(context.queue_complete_semaphores);
    context.queue_complete_semaphores = 0;

    darray_destroy(context.in_flight_fences);
    context.in_flight_fences = 0;

    darray_destroy(context.images_in_flight);
    context.images_in_flight = 0;

    // destroy the command buffers
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {
        if (context.graphics_command_buffers[i].handle) {
            vulkan_command_buffer_free_from_pool(&context, context.device.graphics_command_pool, &context.graphics_command_buffers[i]);
            context.graphics_command_buffers[i].handle = 0;
        }
    }
    darray_destroy(context.graphics_command_buffers);
    context.graphics_command_buffers = 0;

    // Destroy framebuffers and the framebuffers array
    if (context.swapchain.framebuffers) {
        for (u32 i = 0; i < context.swapchain.image_count; ++i) {
            vulkan_framebuffer_destroy(&context, &context.swapchain.framebuffers[i]);
        }
        darray_destroy(context.swapchain.framebuffers);
        context.swapchain.framebuffers = 0;
    }

    vulkan_renderpass_destroy(&context, &context.main_renderpass);

    vulkan_swapchain_destroy(&context, &context.swapchain);

    vulkan_device_destroy(&context);

    if (context.surface) {
        vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
        context.surface = 0;
    }

    if (context.debug_messenger) {
        PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func) {
            func(context.instance, context.debug_messenger, context.allocator);
        }
    }


    vkDestroyInstance(context.instance, context.allocator);
}

void vulkan_renderer_backend_resized(struct renderer_backend *backend, u16 width, u16 height) {
    cached_framebuffer_width = width;
    cached_framebuffer_height = height;

    context.framebuffer_size_generation++;
    LOG_DEBUG("Renderer backend called to resize to %dx%d. New generation %d", width, height, context.framebuffer_size_generation);
}

b8 vulkan_renderer_backend_begin_frame(struct renderer_backend *backend, f32 delta_time) {
    vulkan_device* device = &context.device;

    // Chekc if reacreating swapchain. If so, wait for the device to be idle and boot out
    if (context.recreating_swapchain) {
        VkResult result = vkDeviceWaitIdle(device->logical);
        if (!vulkan_result_is_success(result)) {
            LOG_ERROR("vulkan_renderer_backend_begin_frame: Failed to wait for the device to be idle");
            return false;
        }
        LOG_DEBUG("Cannot begin frame, recreating swapchain");
        return false;
    }

    // chekc if the framebuffer size has changed. if so, a new swapchain must be created
    if (context.framebuffer_size_generation != context.framebuffer_size_last_generation) {
        VkResult result = vkDeviceWaitIdle(device->logical);
        if (!vulkan_result_is_success(result)) {
            LOG_ERROR("vulkan_renderer_backend_begin_frame: Failed to wait for the device to be idle");
            return false;
        }

        // if the swapchain recreation failed (for example window was minimized), we boot out before unsetting the flag
        if (!recreate_swapchain(backend)) {
            return false;
        }

        LOG_DEBUG("Resized the swapchain, booting out of the frame");
        return false;
    }

    // wait for the execution of the current frame to complete. the fence being free will allow this one to move on.
    if (!vulkan_fence_wait(
        &context,
        &context.in_flight_fences[context.current_frame],
        UINT64_MAX)) {
        LOG_WARN("Failed to wait for the fence to be free");
    }

    // Image available and swapchain ready. We can start rendering

    // acquire the next image from the swpachain
    if (!vulkan_swapchain_acquire_next_image_index(
        &context,
        &context.swapchain,
        UINT64_MAX,
        context.image_available_semaphores[context.current_frame],
        0,
        &context.image_index)) {
        LOG_ERROR("Failed to acquire the next image index");
        return false;
    }

    // begin recording commands
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];
    vulkan_command_buffer_reset(command_buffer);
    vulkan_command_buffer_begin_recording(command_buffer, false, false, false);

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = (f32)context.framebuffer_height;
    viewport.width = (f32)context.framebuffer_width;
    viewport.height = -(f32)context.framebuffer_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = context.framebuffer_width;
    scissor.extent.height = context.framebuffer_height;

    vkCmdSetViewport(command_buffer->handle, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer->handle, 0, 1, &scissor);

    context.main_renderpass.w = context.framebuffer_width;
    context.main_renderpass.h = context.framebuffer_height;

    //LOG_TRACE("Begin render pass on frame buffer %p (with image view %p)", &context.swapchain.framebuffers[context.image_index], context.swapchain.views[context.image_index]);
    vulkan_renderpass_begin(command_buffer, &context.main_renderpass, context.swapchain.framebuffers[context.image_index].handle);

    return true;
}

b8 vulkan_renderer_backend_end_frame(struct renderer_backend *backend, f32 delta_time) {
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];

    vulkan_renderpass_end(command_buffer, &context.main_renderpass);
    vulkan_command_buffer_end_recording(command_buffer);

    // Make sure the previous frame is not using this iamge (ie its fence is being waited on)
    if (context.images_in_flight[context.image_index] != VK_NULL_HANDLE) {
        vulkan_fence_wait(
            &context,
            context.images_in_flight[context.image_index],
            UINT64_MAX);
    }

    context.images_in_flight[context.image_index] = &context.in_flight_fences[context.current_frame];

    vulkan_fence_reset(&context, &context.in_flight_fences[context.current_frame]);

    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer->handle;

    // semaphore to signal when the queue is complete
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &context.queue_complete_semaphores[context.current_frame];

    // wait semaphore ensure that the operation cannot beign until the image is availabel
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &context.image_available_semaphores[context.current_frame];

    // each semaphore waits on the corresponding pipeline stage to complete. this flags prevents subsequent color attachment writes
    // from executing until the semaphore signals (one frame is presented at a time)
    VkPipelineStageFlags flags[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.pWaitDstStageMask = flags;

    VkResult result = vkQueueSubmit(
        context.device.graphics_queue,
        1,
        &submit_info,
        context.in_flight_fences[context.current_frame].handle);

    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to submit the queue: %s", vulkan_result_string(result, true));
        return false;
    }

    vulkan_command_buffer_update_submitted(command_buffer);


    // finally give the image backe to the swapchain
    vulkan_swapchain_present(
        &context,
        &context.swapchain,
        context.device.graphics_queue,
        context.device.present_queue,
        context.queue_complete_semaphores[context.current_frame],
        context.image_index);

    return true;
}


VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {
    switch (message_severity) {
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            LOG_ERROR(callback_data->pMessage);
        break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            LOG_WARN(callback_data->pMessage);
        break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            LOG_DEBUG(callback_data->pMessage);
        break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            LOG_TRACE(callback_data->pMessage);
        break;
    }
    return VK_FALSE;
}

i32 find_memory_index(u32 type_filter, u32 property_flags) {
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(context.device.physical, &memory_properties);

    for (u32 i = 0; i < memory_properties.memoryTypeCount; ++i) {
        if (type_filter & (1 << i) && (memory_properties.memoryTypes[i].propertyFlags & property_flags) == property_flags) {
            return i;
        }
    }

    LOG_WARN("Failed to find a suitable memory type");
    return -1;
}

void create_command_buffers(renderer_backend *backend) {
    if (!context.graphics_command_buffers) {
        context.graphics_command_buffers = darray_reserve(vulkan_command_buffer, context.swapchain.image_count);
        for (u32 i = 0; i < context.swapchain.image_count; ++i) {
            czero_memory(&context.graphics_command_buffers[i], sizeof(vulkan_command_buffer));
        }
    }

    for (u32 i = 0; i < context.swapchain.image_count; ++i) {
        if (context.graphics_command_buffers[i].handle) {
            vulkan_command_buffer_free_from_pool(&context, context.device.graphics_command_pool, &context.graphics_command_buffers[i]);
        }
        czero_memory(&context.graphics_command_buffers[i], sizeof(vulkan_command_buffer));
        vulkan_command_buffer_allocate_from_pool(&context, context.device.graphics_command_pool, true, &context.graphics_command_buffers[i]);
    }
}

void regenerate_framebuffers(renderer_backend *backend, vulkan_swapchain *swapchain, vulkan_renderpass *renderpass) {
    for (u32 i = 0; i < swapchain->image_count; ++i) {
        u32 attachment_count = 2;
        VkImageView attachments[] = {
            swapchain->views[i],
            swapchain->depth_attachment.view
        };

        vulkan_framebuffer_create(
            &context,
            renderpass,
            context.framebuffer_width,
            context.framebuffer_height,
            attachment_count,
            attachments,
            &swapchain->framebuffers[i]);

        LOG_TRACE("Regenerating framebuffer %d (frambuffer pointer %p | image view pointer %p)", i, &swapchain->framebuffers[i], swapchain->views[i]);
    }
}

b8 recreate_swapchain(renderer_backend *backend) {
    LOG_DEBUG("recreate_swapchain: Recreating the swapchain");
    if (context.recreating_swapchain) {
        LOG_DEBUG("recreate_swapchain: Already recreating the swapchain");
        return false;
    }

    if (context.framebuffer_width == 0 || context.framebuffer_height == 0) {
        LOG_DEBUG("recreate_swapchain: window is too small in a dimension to recreate the swapchain");
        return false;
    }

    context.recreating_swapchain = true;

    vkDeviceWaitIdle(context.device.logical);

    // clear all images in flight
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {
        // todo: maybe wait these fences?
        context.images_in_flight[i] = 0;
    }

    // requery swapchain support info
    vulkan_device_query_swapchain_support(context.device.physical, context.surface, &context.device.swapchain_support_info);
    vulkan_device_detect_depth_format(&context.device);

    vulkan_swapchain_recreate(
        &context,
        cached_framebuffer_width,
        cached_framebuffer_height,

        &context.swapchain);
    context.framebuffer_width = context.swapchain.extent.width;
    context.framebuffer_height = context.swapchain.extent.height;

    // sync the framebuffer size with the cached sizes
    context.framebuffer_width = cached_framebuffer_width;
    context.framebuffer_height = cached_framebuffer_height;
    context.main_renderpass.w = context.framebuffer_width;
    context.main_renderpass.h = context.framebuffer_height;
    cached_framebuffer_width = 0;
    cached_framebuffer_height = 0;

    context.framebuffer_size_last_generation = context.framebuffer_size_generation;

    // cleanup command buffers
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {
        vulkan_command_buffer_free_from_pool(&context, context.device.graphics_command_pool, &context.graphics_command_buffers[i]);
    }

    // destroy framebuffers
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {
        vulkan_framebuffer_destroy(&context, &context.swapchain.framebuffers[i]);
    }

    context.main_renderpass.x = 0;
    context.main_renderpass.y = 0;
    context.main_renderpass.w = context.framebuffer_width;
    context.main_renderpass.h = context.framebuffer_height;

    vulkan_swapchain_regenerate_framebuffers(&context, &context.swapchain, &context.main_renderpass);

    create_command_buffers(backend);

    context.recreating_swapchain = false;

    return true;
}

