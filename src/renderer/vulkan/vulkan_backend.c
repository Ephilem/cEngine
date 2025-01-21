#include "vulkan_backend.h"

#include "vulkan_device.h"
#include "vulkan_platform.h"
#include "vulkan_types.inl"

#include "core/logger.h"
#include "core/cstring.h"

#include "containers/darray.h"

#include "platform/platform.h"

// static vulkan context
static vulkan_context context;

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data);


b8 vulkan_renderer_backend_initialize(struct renderer_backend *backend, const char *application_name, struct platform_state *platform_state) {

    // TODO use platform allocator with the tag MEMORY_TAG_VULKAN
    context.allocator = 0;

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

    LOG_DEBUG("Required vulkan extensions:");
    u32 length = darray_length(required_extensions);
    for (u32 i = 0; i < length; i++) {
        LOG_DEBUG(required_extensions[i]);
    }
#endif
    create_info.enabledExtensionCount = darray_length(required_extensions);
    create_info.ppEnabledExtensionNames = required_extensions;

    const char** required_validation_layer_names = 0;
    u32 required_validation_layer_count = 0;

#if _DEBUG
    LOG_INFO("Loading vulkan validation layers for debugging");

    required_validation_layer_names = darray_create(const char*);
    darray_push(required_validation_layer_names, &"VK_LAYER_KHRONOS_validation");
    required_validation_layer_count = darray_length(required_validation_layer_names);

    // fetch required validation layers
    u32 available_layer_count = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, 0));
    VkLayerProperties* available_layers = darray_reserve(VkLayerProperties, available_layer_count);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers));

    for (u32 i = 0; i < required_validation_layer_count; ++i) {
        LOG_INFO("Searching for layer : %s", required_validation_layer_names[i]);
        b8 found = FALSE;
        for (u32 j = 0; j < available_layer_count; ++j) {
            if (string_equals(required_validation_layer_names[i], available_layers[j].layerName) == 0) {
                found = TRUE;
                LOG_INFO("Found!");
                break;
            }
        }

        if (!found) {
            LOG_FATAL("The required layer %s is not available", required_validation_layer_names[i]);
            return FALSE;
        }
    }
    LOG_INFO("All validation layers found");
#endif

    create_info.enabledLayerCount = required_validation_layer_count;
    create_info.ppEnabledLayerNames = required_validation_layer_names;

    VK_CHECK(vkCreateInstance(&create_info, NULL, &context.instance));

#if _DEBUG
    LOG_INFO("Creating vulkan debugger");
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

    if (!platform_create_vulkan_surface(platform_state, &context)) {
        LOG_ERROR("Failed to create the vulkan surface");
        return FALSE;
    }

    if (!vulkan_device_create(&context)) {
        LOG_ERROR("Failed to create the device");
        return FALSE;
    }
    return TRUE;
}

void vulkan_renderer_backend_shutdown(struct renderer_backend *backend) {
    if (context.debug_messenger) {
        PFN_vkDestroyDebugUtilsMessengerEXT func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func) {
            func(context.instance, context.debug_messenger, context.allocator);
        }
    }

    vulkan_device_destroy(&context);
    vkDestroyInstance(context.instance, context.allocator);

}

void vulkan_renderer_backend_resized(struct renderer_backend *backend, u16 width, u16 height) {
}

b8 vulkan_renderer_backend_begin_frame(struct renderer_backend *backend, f32 delta_time) {
    return TRUE;
}

b8 vulkan_renderer_backend_end_frame(struct renderer_backend *backend, f32 delta_time) {
    return TRUE;
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
            LOG_INFO(callback_data->pMessage);
        break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            LOG_TRACE(callback_data->pMessage);
        break;
    }
    return VK_FALSE;
}


