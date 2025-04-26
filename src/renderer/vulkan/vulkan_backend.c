#include "vulkan_backend.h"

#include "vulkan_buffer.h"
#include "vulkan_device.h"
#include "vulkan_platform.h"
#include "vulkan_swapchain.h"
#include "vulkan_renderpass.h"
#include "vulkan_command_buffer.h"
#include "vulkan_framebuffer.h"
#include "vulkan_fence.h"
#include "vulkan_image.h"
#include "vulkan_utils.h"
#include "vulkan_types.inl"

#include "core/logger.h"
#include "core/cstring.h"
#include "core/cmemory.h"

#include "containers/darray.h"
#include "core/application.h"
#include "math/math_types.h"

#include "platform/platform.h"

#include "shaders/vulkan_material_shader.h"
#include "systems/material_system.h"

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
b8 create_buffers(vulkan_context* context);

void create_command_buffers(renderer_backend* backend);
void regenerate_framebuffers(renderer_backend* backend, vulkan_swapchain* swapchain, vulkan_renderpass* renderpass);
b8 recreate_swapchain(renderer_backend* backend);


// TEMP METHOD
void upload_data_range(vulkan_context* context, VkCommandPool pool, VkFence fence, VkQueue queue, vulkan_buffer* buffer, u64 offset, u64 size, const void* data) {
    VkBufferUsageFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    // Because we cant write directly to the buffer (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), we need to create a staging buffer
    vulkan_buffer staging;
    vulkan_buffer_create(context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, flags, true, &staging);

    vulkan_buffer_load_data(context, &staging, 0, size, 0, data);

    vulkan_buffer_copy_to(context, pool, fence, queue, staging.handle, 0, buffer->handle, offset, size);

    vulkan_buffer_destroy(context, &staging);
}

void free_data_range(vulkan_buffer* buffer, u64 offset, u64 size) {
    // TODO free this in the buffer
    // TODO update free list with this range being free
}

b8 vulkan_backend_initialize(struct renderer_backend *backend, const char *application_name, struct platform_state *platform_state) {
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

    // create builtin shaders
    if (!vulkan_material_shader_create(&context, &context.material_shader)) {
        LOG_ERROR("Failed to create the vulkan object shader");
        return false;
    }

    create_buffers(&context);

    // mark all geometries as invalid
    for (u32 i = 0; i < VULKAN_MAX_GEOMETRY_COUNT; ++i) {
        context.geometries[i].id = INVALID_ID;
    }

    LOG_INFO("Vulkan renderer backend initialized");
    return true;
}

void vulkan_backend_shutdown(struct renderer_backend *backend) {
    vkDeviceWaitIdle(context.device.logical);

    // destroy buffers
    vulkan_buffer_destroy(&context, &context.object_vertex_buffer);
    vulkan_buffer_destroy(&context, &context.object_index_buffer);

    vulkan_material_shader_destroy(&context, &context.material_shader);

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

void vulkan_backend_resized(struct renderer_backend *backend, u16 width, u16 height) {
    cached_framebuffer_width = width;
    cached_framebuffer_height = height;

    context.framebuffer_size_generation++;
    LOG_DEBUG("Renderer backend called to resize to %dx%d. New generation %d", width, height, context.framebuffer_size_generation);
}

b8 vulkan_backend_begin_frame(struct renderer_backend *backend, f32 delta_time) {
    vulkan_device* device = &context.device;
    context.frame_delta_time = delta_time;

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

    if (context.images_in_flight[context.image_index]) {
        vulkan_fence_wait(&context, context.images_in_flight[context.image_index], UINT64_MAX);
    }

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

void vulkan_backend_update_global_state(mat4 projection, mat4 view, vec3 view_position, vec4 ambient_colour, i32 mode) {
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];
    vulkan_material_shader_use(&context, &context.material_shader);

    // copy parameters so we sure that the data is immutable (no pointer
    context.material_shader.global_ubo.projection = projection;
    context.material_shader.global_ubo.view = view;

    // todo : other ubo property

    vulkan_material_shader_update_global_state(&context, &context.material_shader, context.frame_delta_time);

    // mark all ubo as need to be updated
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {
        context.material_shader.global_descriptor_updated[i] = false;
    }
}

b8 vulkan_backend_end_frame(struct renderer_backend *backend, f32 delta_time) {
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

b8 create_buffers(vulkan_context* context) {
    VkMemoryPropertyFlagBits memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; // because its local only, this memory is faster

    // vertex buffer
    const u64 vertex_buffer_size = sizeof(vertex_3d) * 1024 * 1024;
    if (!vulkan_buffer_create(
            context,
            vertex_buffer_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            memory_property_flags,
            true,
            &context->object_vertex_buffer)) {

        LOG_ERROR("Error creating vertex buffer.");
        return false;
    }
    context->geometry_vertex_offset = 0;

    // index buffer
    const u64 index_buffer_size = sizeof(u32) * 1024 * 1024;
    if (!vulkan_buffer_create(
            context,
            index_buffer_size,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            memory_property_flags,
            true,
            &context->object_index_buffer)) {

        LOG_ERROR("Error creating index buffer.");
        return false;
    }
    context->geometry_index_offset = 0;

    return true;
}

void vulkan_backend_create_texture(const u8 *pixels, struct texture *texture) {

    // TODO use allocator
    texture->internal_data = (vulkan_texture_data*)callocate(sizeof(vulkan_texture_data), MEMORY_TAG_TEXTURE);
    vulkan_texture_data* data = (vulkan_texture_data*)texture->internal_data;
    VkDeviceSize image_size = texture->width * texture->height * texture->channel_count;

    // assuming 8 bits per channel
    VkFormat image_format = VK_FORMAT_R8G8B8A8_UNORM;

    // staging buffer to upload the image - loading data in this buffer
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags memory_prop_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    vulkan_buffer staging;
    vulkan_buffer_create(&context, image_size, usage, memory_prop_flags, true, &staging);
    vulkan_buffer_load_data(&context, &staging, 0, image_size, 0, pixels);

    // NOTE : we assure a single type of texture, different texture types will require
    // different options here (ex 3D textures)
    vulkan_image_create(
        &context,
        VK_IMAGE_TYPE_2D,
        texture->width, texture->height,
        image_format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        true,
        VK_IMAGE_ASPECT_COLOR_BIT,
        &data->image);

    // transition the layout from whatevent to optimal for recieving data (for the gpu)
    vulkan_command_buffer temp_buffer;
    VkCommandPool pool = context.device.graphics_command_pool;
    VkQueue queue = context.device.graphics_queue;
    vulkan_command_buffer_allocate_and_begin_single_use(&context, pool, &temp_buffer);
    vulkan_image_transition_layout(
        &context,
        &temp_buffer,
        &data->image,
        image_format,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // copy the data from the buffer
    vulkan_image_copy_from_buffer(&context, &data->image, staging.handle, &temp_buffer);

    // set the transferred image to the shader read only optimal layout
    vulkan_image_transition_layout(
        &context,
        &temp_buffer,
        &data->image,
        image_format,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vulkan_command_buffer_end_single_use(&context, pool, &temp_buffer, queue);

    vulkan_buffer_destroy(&context, &staging);

    // create the sampler for the texture (used for sampling the texture in the shader, so the shader can use the texture)
    VkSamplerCreateInfo sampler_create_info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    // define filter if the image is magnified or minified
    sampler_create_info.magFilter = VK_FILTER_NEAREST;
    sampler_create_info.minFilter = VK_FILTER_NEAREST;
    // repeat the texture if the uv coordinates are outside the texture
    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    // anisotropic filtering improves the quality of the texture when viewed at an angle
    sampler_create_info.anisotropyEnable = VK_TRUE;
    sampler_create_info.maxAnisotropy = 16;
    sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;
    sampler_create_info.compareEnable = VK_FALSE;
    sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_create_info.mipLodBias = 0.0f;
    sampler_create_info.minLod = 0.0f;
    sampler_create_info.maxLod = 0.0f;

    VkResult result = vkCreateSampler(context.device.logical, &sampler_create_info, context.allocator, &data->sampler);
    if (!vulkan_result_is_success(VK_SUCCESS)) {
        LOG_ERROR("Failed to create the texture sampler: %s", vulkan_result_string(result, true));
        return;
    }

    texture->generation++; // increment the generation of the texture. this is used to check if the texture has been modified (ex anisotropic details changed)
}

void vulkan_backend_destroy_texture(texture* texture) {
    vkDeviceWaitIdle(context.device.logical);
    vulkan_texture_data* data = (vulkan_texture_data*)texture->internal_data;

    if (data) {
        vulkan_image_destroy(&context, &data->image);
        czero_memory(&data->image, sizeof(vulkan_image));
        vkDestroySampler(context.device.logical, data->sampler, context.allocator);
        data->sampler = 0;

        cfree(texture->internal_data, sizeof(vulkan_texture_data), MEMORY_TAG_TEXTURE);
        czero_memory(texture, sizeof(struct texture));
    }
}

b8 vulkan_backend_create_material(struct material* material) {
    if (material) {
        if (!vulkan_material_shader_acquire_resources(&context, &context.material_shader, material)) {
            LOG_ERROR("Failed to acquire resources for the material shader");
            return false;
        }

        LOG_TRACE("Material created with id %d", material->id);
        return true;
    }

    LOG_ERROR("vulkan_backend_create_material: material is null");
    return false;
}

void vulkan_backend_destroy_material(struct material* material) {
    if (material) {
        if (material->internal_id != INVALID_ID) {
            vulkan_material_shader_release_resources(&context, &context.material_shader, material);
            material->internal_id = INVALID_ID;
        } else {
            LOG_WARN("vulkan_backend_destroy_material: material is not valid");
        }
    } else {
        LOG_WARN("vulkan_backend_destroy_material: material is null. nothing done");
    }
}

b8 vulkan_backend_create_geometry(geometry* geometry, u32 vertex_count, const vertex_3d* vertices, u32 index_count, const u32* indices) {
    if (!vertex_count || !vertices) {
        LOG_ERROR("vulkan_backend_create_geometry: vertex count is 0 or vertices is null");
        return false;
    }

    // check if this is a re-upload. If so, we need to free old data afterward
    b8 reupload = geometry->internal_id != INVALID_ID;
    vulkan_geometry_data old_range;

    vulkan_geometry_data* internal_data = 0;
    if (reupload) {
        internal_data = &context.geometries[geometry->internal_id];

        // copy the old range
        old_range.index_buffer_offset = internal_data->index_buffer_offset;
        old_range.index_count = internal_data->index_count;
        old_range.index_size = internal_data->index_size;
        old_range.vertex_buffer_offset = internal_data->vertex_buffer_offset;
        old_range.vertex_count = internal_data->vertex_count;
        old_range.vertex_size = internal_data->vertex_size;
    } else {
        // find a free index
        for (u32 i = 0; i < VULKAN_MAX_GEOMETRY_COUNT; ++i) {
            if (context.geometries[i].id == INVALID_ID) {
                geometry->internal_id = i;
                context.geometries[i].id = i;
                internal_data = &context.geometries[i];
                break;
            }
        }
    }

    if (!internal_data) {
        LOG_FATAL("vulkan_backend_create_geometry: no free geometry slots available");
        return false;
    }

    VkCommandPool pool = context.device.graphics_command_pool;
    VkQueue queue = context.device.graphics_queue;

    // Vertex data
    internal_data->vertex_buffer_offset = context.geometry_vertex_offset;
    internal_data->vertex_count = vertex_count;
    internal_data->vertex_size = sizeof(vertex_3d) * vertex_count;
    upload_data_range(
        &context,
        pool,
        0,
        queue,
        &context.object_vertex_buffer,
        internal_data->vertex_buffer_offset,
        internal_data->vertex_size,
        vertices);
    context.geometry_vertex_offset += internal_data->vertex_size;

    // Index data, if applicable
    if (index_count && indices) {
        internal_data->index_buffer_offset = context.geometry_index_offset;
        internal_data->index_count = index_count;
        internal_data->index_size = sizeof(u32) * index_count;
        upload_data_range(
            &context,
            pool,
            0,
            queue,
            &context.object_index_buffer,
            internal_data->index_buffer_offset,
            internal_data->index_size,
            indices);
        context.geometry_index_offset += internal_data->index_size;
    }

    if (internal_data->generation == INVALID_ID) {
        internal_data->generation = 0;
    } else {
        internal_data->generation++;
    }

    if (reupload) {
        // free vertex data
        free_data_range(&context.object_vertex_buffer, old_range.vertex_buffer_offset, old_range.vertex_size);

        // free index data, if applicable
        if (old_range.index_size > 0) {
            free_data_range(&context.object_index_buffer, old_range.index_buffer_offset, old_range.index_size);
        }
    }

    return true;
}

void vulkan_backend_destroy_geometry(geometry* geometry) {
    if (geometry && geometry->internal_id != INVALID_ID) {
        vkDeviceWaitIdle(context.device.logical);
        vulkan_geometry_data* internal_data = &context.geometries[geometry->internal_id];

        // free vertex data
        free_data_range(&context.object_vertex_buffer, internal_data->vertex_buffer_offset, internal_data->vertex_size);

        // free index data, if applicable
        if (internal_data->index_size > 0) {
            free_data_range(&context.object_index_buffer, internal_data->index_buffer_offset, internal_data->index_size);
        }

        // clean up data
        czero_memory(internal_data, sizeof(vulkan_geometry_data));
        internal_data->generation = INVALID_ID;
        internal_data->id = INVALID_ID;
    }
}

void vulkan_backend_draw_geometry(geometry_render_data data) {
    if (data.geometry && data.geometry->internal_id == INVALID_ID) {
        return; // no geometry to draw
    }

    vulkan_geometry_data* buffer_data = &context.geometries[data.geometry->internal_id];
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];

    // TODO check if this is actually needed
    vulkan_material_shader_use(&context, &context.material_shader);

    vulkan_material_shader_set_model(&context, &context.material_shader, data.model);

    material* m = 0;
    if (data.geometry->material) {
        m = data.geometry->material;
    } else {
        m = material_system_get_default_material();
    }
    vulkan_material_shader_apply_material(&context, &context.material_shader, m);

    VkDeviceSize offsets[1] = {buffer_data->vertex_buffer_offset};
    vkCmdBindVertexBuffers(command_buffer->handle, 0, 1, &context.object_vertex_buffer.handle, (VkDeviceSize*)offsets);

    // Draw indexed or non-indexed
    if (buffer_data->index_count > 0) {
        vkCmdBindIndexBuffer(
            command_buffer->handle,
            context.object_index_buffer.handle,
            buffer_data->index_buffer_offset,
            VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(command_buffer->handle, buffer_data->index_count, 1, 0, 0, 0);
    } else {
        vkCmdDraw(command_buffer->handle, buffer_data->vertex_count, 1, 0, 0);
    }
}