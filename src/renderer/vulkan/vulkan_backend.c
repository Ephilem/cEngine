#include "vulkan_backend.h"

#include "vulkan_platform.h"
#include "vulkan_types.inl"

#include "core/logger.h"

#include "containers/darray.h"

#include "platform/platform.h"

// static vulkan context
static vulkan_context context;

b8 vulkan_renderer_backend_initialize(struct renderer_backend *backend, const char *application_name, struct platform_state *platform_state) {

    // TODO use platform allocator with the tag MEMORY_TAG_VULKAN
    context.allocator = 0;

    VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.apiVersion = VK_API_VERSION_1_4;
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

    const char** required_validation_layer_names = darray_create(required_extensions);
    u32 required_validation_layer_count = 0;



    create_info.enabledLayerCount = 0;
    create_info.ppEnabledLayerNames = 0;

    if (vkCreateInstance(&create_info, NULL, &context.instance) != VK_SUCCESS) {
        LOG_ERROR("Failed to create Vulkan instance");
        return FALSE;
    }

    return TRUE;
}

void vulkan_renderer_backend_shutdown(struct renderer_backend *backend) {
}

void vulkan_renderer_backend_resized(struct renderer_backend *backend, u16 width, u16 height) {
}

b8 vulkan_renderer_backend_begin_frame(struct renderer_backend *backend, f32 delta_time) {
    return TRUE;
}

b8 vulkan_renderer_backend_end_frame(struct renderer_backend *backend, f32 delta_time) {
    return TRUE;
}


