#include "vulkan_device.h"

#include "core/cmemory.h"
#include "core/cstring.h"
#include "core/logger.h"

#include "containers/darray.h"

typedef struct vulkan_physical_device_requirements {
    b8 graphics;
    b8 present;
    b8 compute;
    b8 transfer;
    // darray
    const char** device_extension_names;
    b8 sampler_anisotropy;
    b8 discrete_gpu;
} vulkan_physical_device_requirements;

typedef struct vulkan_physical_device_queue_family_info {
    u32 graphics_family_index;
    u32 present_family_index;
    u32 compute_family_index;
    u32 transfer_family_index;
} vulkan_physical_device_queue_family_info;

b8 select_physical_device(vulkan_context* context);
b8 physical_device_meets_requirements(
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    const VkPhysicalDeviceProperties* properties,
    const VkPhysicalDeviceFeatures* features,
    const vulkan_physical_device_requirements* requirements,
    vulkan_physical_device_queue_family_info* out_queue_info,
    vulkan_swapchain_support_info* out_swapchain_support_info);


b8 vulkan_device_create(vulkan_context *context) {
    LOG_DEBUG("Creating Vulkan device");

    // Choosing a physical device by their properties
    if (!select_physical_device(context)) {
        LOG_ERROR("Failed to select a physical device");
        return FALSE;
    }

    b8 present_shares_graphics_queue = context->device.graphics_queue_index == context->device.present_queue_index;
    b8 transfer_shares_graphics_queue = context->device.graphics_queue_index == context->device.transfer_queue_index;
    u32 index_count = 1;

    if (!present_shares_graphics_queue) {
        index_count++;
    }
    if (!transfer_shares_graphics_queue) {
        index_count++;
    }

    u32 indices[index_count];
    u8 index = 0;
    indices[index++] = context->device.graphics_queue_index;
    if (!present_shares_graphics_queue) {
        indices[index++] = context->device.present_queue_index;
    }
    if (!transfer_shares_graphics_queue) {
        indices[index++] = context->device.transfer_queue_index;
    }

    VkDeviceQueueCreateInfo queue_create_infos[index_count];
    for (u32 i = 0; i < index_count; ++i) {
        queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[i].queueFamilyIndex = indices[i];
        queue_create_infos[i].queueCount = 1;
        /*if (indices[i] == context->device.graphics_queue_index) {
            queue_create_infos[i].queueCount = 2;
        }*/
        queue_create_infos[i].flags = 0;
        queue_create_infos[i].pNext = 0;
        f32 queue_priority = 1.0f;
        queue_create_infos[i].pQueuePriorities = &queue_priority;
    }


    VkPhysicalDeviceFeatures device_features = {};
    device_features.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo create_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    create_info.queueCreateInfoCount = index_count;
    create_info.pQueueCreateInfos = queue_create_infos;
    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount = 1;
    const char* extension_names = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    create_info.ppEnabledExtensionNames = &extension_names;

    LOG_DEBUG("Creating the logical device with %d queue families", index_count);
    VK_CHECK(vkCreateDevice(context->device.physical, &create_info, context->allocator, &context->device.logical));

    vkGetDeviceQueue(
        context->device.logical,
        context->device.graphics_queue_index,
        0,
        &context->device.graphics_queue);

    vkGetDeviceQueue(
        context->device.logical,
        context->device.present_queue_index,
        0,
        &context->device.present_queue);

    vkGetDeviceQueue(
        context->device.logical,
        context->device.compute_queue_index,
        0,
        &context->device.compute_queue);

    vkGetDeviceQueue(
        context->device.logical,
        context->device.transfer_queue_index,
        0,
        &context->device.transfer_queue);

    LOG_DEBUG("Creating a command pool for the graphics queue");
    VkCommandPoolCreateInfo command_pool_create_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    command_pool_create_info.queueFamilyIndex = context->device.graphics_queue_index;
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(vkCreateCommandPool(context->device.logical, &command_pool_create_info, context->allocator, &context->device.graphics_command_pool));

    return TRUE;
}

void vulkan_device_destroy(vulkan_context *context) {

    context->device.graphics_queue = 0;
    context->device.present_queue = 0;
    context->device.compute_queue = 0;
    context->device.transfer_queue = 0;

    vkDestroyCommandPool(context->device.logical, context->device.graphics_command_pool, context->allocator);

    if (context->device.logical) {
        vkDestroyDevice(context->device.logical, context->allocator);
        context->device.logical = 0;
    }

    context->device.physical = 0;

    if (context->device.swapchain_support_info.formats) {
        cfree(context->device.swapchain_support_info.formats, context->device.swapchain_support_info.format_count * sizeof(VkSurfaceFormatKHR), MEMORY_TAG_RENDERER);
        context->device.swapchain_support_info.formats = 0;
        context->device.swapchain_support_info.format_count = 0;
    }

    if (context->device.swapchain_support_info.present_modes) {
        cfree(context->device.swapchain_support_info.present_modes, context->device.swapchain_support_info.present_mode_count * sizeof(VkPresentModeKHR), MEMORY_TAG_RENDERER);
        context->device.swapchain_support_info.present_modes = 0;
        context->device.swapchain_support_info.present_mode_count = 0;
    }

    czero_memory(
        &context->device.swapchain_support_info.capabilities,
        sizeof(context->device.swapchain_support_info.capabilities));

    context->device.graphics_queue_index = -1;
    context->device.present_queue_index = -1;
    context->device.compute_queue_index = -1;
    context->device.transfer_queue_index = -1;
}

void vulkan_device_query_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface, vulkan_swapchain_support_info *info) {
    // surface capabilities
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &info->capabilities));

    // surface formats
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &info->format_count, 0));

    if (info->format_count != 0) {
        if (!info->formats) {
            info->formats = callocate(info->format_count * sizeof(VkSurfaceFormatKHR), MEMORY_TAG_RENDERER);
        }
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &info->format_count, info->formats));
    }

    // presents modes
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &info->present_mode_count, 0));

    if (info->present_mode_count != 0) {
        if (!info->present_modes) {
            info->present_modes = callocate(info->present_mode_count * sizeof(VkPresentModeKHR), MEMORY_TAG_RENDERER);
        }
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &info->present_mode_count, info->present_modes));
    }
}

b8 select_physical_device(vulkan_context *context) {
    u32 physical_device_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_device_count, NULL));
    if (physical_device_count == 0) {
        LOG_ERROR("Failed to find any physical devices that support Vulkan");
        return FALSE;
    }
    VkPhysicalDevice physical_devices[physical_device_count];
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_device_count, physical_devices));


    for (u32 i = 0; i < physical_device_count; i++) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical_devices[i], &properties);

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(physical_devices[i], &features);

        VkPhysicalDeviceMemoryProperties memory;
        vkGetPhysicalDeviceMemoryProperties(physical_devices[i], &memory);

        vulkan_physical_device_requirements requirements = {
            .graphics = TRUE,
            .present = TRUE,
            .compute = TRUE,
            .transfer = TRUE,
            .sampler_anisotropy = TRUE,
            .discrete_gpu = TRUE,
            .device_extension_names = darray_create(const char*)
        };
        darray_push(requirements.device_extension_names, &VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        vulkan_physical_device_queue_family_info queue_info = {};
        b8 result = physical_device_meets_requirements(
            physical_devices[i],
            context->surface,
            &properties,
            &features,
            &requirements,
            &queue_info,
            &context->device.swapchain_support_info);

        // if we can use this device
        if (result) {
            LOG_TRACE("Selected device: %s", properties.deviceName);
            switch (properties.deviceType) {
                default:
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    LOG_TRACE("Device Type: Other");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    LOG_TRACE("Device Type: Integrated GPU");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    LOG_TRACE("Device Type: Discrete GPU");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    LOG_TRACE("Device Type: Virtual GPU");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    LOG_TRACE("Device Type: CPU");
                    break;
            }

            LOG_TRACE(
                "GPU Driver version: %d.%d.%d",
                VK_VERSION_MAJOR(properties.driverVersion),
                VK_VERSION_MINOR(properties.driverVersion),
                VK_VERSION_PATCH(properties.driverVersion));

            LOG_TRACE(
                "Vulkan API version: %d.%d.%d",
                VK_VERSION_MAJOR(properties.apiVersion),
                VK_VERSION_MINOR(properties.apiVersion),
                VK_VERSION_PATCH(properties.apiVersion));


            LOG_TRACE("=== Memory Types ===");
            LOG_TRACE("Type          | Size (GiB) | Location");
            LOG_TRACE("--------------|------------|----------");
            for (u32 j = 0; j < memory.memoryTypeCount; ++j) {
                f32 memory_size_gib = ((f32) memory.memoryHeaps[memory.memoryTypes[j].heapIndex].size) / (1024 * 1024 * 1024);
                const char* location = (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) ? "GPU" : "System";
                const char* type = (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) ? "Device Local " : "Shared System";
                LOG_TRACE("%-4s | %10.2f | %s", type, memory_size_gib, location);
            }
            LOG_TRACE("=====================");

            context->device.physical = physical_devices[i];
            context->device.graphics_queue_index = queue_info.graphics_family_index;
            context->device.present_queue_index = queue_info.present_family_index;
            context->device.compute_queue_index = queue_info.compute_family_index;
            context->device.transfer_queue_index = queue_info.transfer_family_index;

            context->device.properties = properties;
            context->device.features = features;
            context->device.memory = memory;
            break;
        }
    }

    // ensure we found a device
    if (!context->device.physical) {
        LOG_ERROR("Failed to find a suitable physical device");
        return FALSE;
    }

    return TRUE;
}

b8 physical_device_meets_requirements(VkPhysicalDevice device, VkSurfaceKHR surface,
    const VkPhysicalDeviceProperties *properties, const VkPhysicalDeviceFeatures *features,
    const vulkan_physical_device_requirements *requirements, vulkan_physical_device_queue_family_info *out_queue_info,
    vulkan_swapchain_support_info *out_swapchain_support_info) {

    // Set an insane value to check if it was set
    out_queue_info->graphics_family_index = -1;
    out_queue_info->present_family_index = -1;
    out_queue_info->compute_family_index = -1;
    out_queue_info->transfer_family_index = -1;

    if (requirements->discrete_gpu) {
        if (properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            LOG_ERROR("Device is not a discrete GPU, and one is required");
            return FALSE;
        }
    }

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);
    VkQueueFamilyProperties* queue_families = darray_reserve(VkQueueFamilyProperties, queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

    LOG_TRACE("Number of queue families: %d", queue_family_count);
    u8 min_transfer_score = 255;
    // Looking at each queue and see what queues it supports
    for (u32 i = 0; i < queue_family_count; ++i) {
        u8 score = 0;

        // Graphics queue?
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            score++;
            out_queue_info->graphics_family_index = i;
        }

        // Compute queue?
        if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            score++;
            out_queue_info->compute_family_index = i;
        }

        // Transfer queue?
        if (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            score++;
            out_queue_info->transfer_family_index = i;
            if (queue_families[i].queueCount < min_transfer_score) {
                min_transfer_score = queue_families[i].queueCount;
            }
        }

        // Present queue?
        VkBool32 present_support = VK_FALSE;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support));
        if (present_support) {
            score++;
            out_queue_info->present_family_index = i;
        }
    }

    LOG_TRACE("Graphics | Compute | Transfer | Present | Name");
    LOG_TRACE("    %d    |    %d    |    %d     |    %d    | %s",
        out_queue_info->graphics_family_index != -1,
        out_queue_info->compute_family_index != -1,
        out_queue_info->transfer_family_index != -1,
        out_queue_info->present_family_index != -1,
        properties->deviceName);

    // check if all the required queues are available
    if (
        (!requirements->graphics || (requirements->graphics && out_queue_info->graphics_family_index != -1)) &&
        (!requirements->compute || (requirements->compute && out_queue_info->compute_family_index != -1)) &&
        (!requirements->transfer || (requirements->transfer && out_queue_info->transfer_family_index != -1)) &&
        (!requirements->present || (requirements->present && out_queue_info->present_family_index != -1))) {

        LOG_TRACE("Device meets all requirements");
        LOG_TRACE("Graphics Family Index: %d", out_queue_info->graphics_family_index);
        LOG_TRACE("Compute Family Index: %d", out_queue_info->compute_family_index);
        LOG_TRACE("Transfer Family Index: %d", out_queue_info->transfer_family_index);
        LOG_TRACE("Present Family Index: %d", out_queue_info->present_family_index);

        vulkan_device_query_swapchain_support(device, surface, out_swapchain_support_info);

        if (out_swapchain_support_info->format_count < 1 || out_swapchain_support_info->present_mode_count < 1) {
            if (out_swapchain_support_info->formats) {
                cfree(out_swapchain_support_info->formats, out_swapchain_support_info->format_count * sizeof(VkSurfaceFormatKHR), MEMORY_TAG_RENDERER);
            }
            if (out_swapchain_support_info->present_modes) {
                cfree(out_swapchain_support_info->present_modes, out_swapchain_support_info->present_mode_count * sizeof(VkPresentModeKHR), MEMORY_TAG_RENDERER);
            }
            LOG_ERROR("Device does not support swapchain");
            return FALSE;
        }
    }

    // Device extensions.
    if (requirements->device_extension_names) {
        u32 available_extension_count = 0;
        VkExtensionProperties* available_extensions = 0;
        VK_CHECK(vkEnumerateDeviceExtensionProperties(device, 0, &available_extension_count, 0));
        if (available_extension_count != 0) {
            available_extensions = callocate(available_extension_count * sizeof(VkExtensionProperties), MEMORY_TAG_RENDERER);
            VK_CHECK(vkEnumerateDeviceExtensionProperties(device, 0, &available_extension_count, available_extensions));

            u32 required_extension_count = darray_length(requirements->device_extension_names);
            for (u32 i = 0; i < required_extension_count; ++i) {
                b8 found = FALSE;
                for (u32 j = 0; j < available_extension_count; ++j) {
                    if (string_equals(requirements->device_extension_names[i], available_extensions[j].extensionName) == 0) {
                        found = TRUE;
                        break;
                    }
                }

                if (!found) {
                    LOG_ERROR("Device does not support the required extension %s", requirements->device_extension_names[i]);
                    return FALSE;
                } else {
                    LOG_TRACE("Device supports extension %s", requirements->device_extension_names[i]);
                }
            }
        }
        cfree(available_extensions, available_extension_count * sizeof(VkExtensionProperties), MEMORY_TAG_RENDERER);
    }

    // sampler anisotropy
    if (requirements->sampler_anisotropy && !features->samplerAnisotropy) {
        LOG_INFO("Device does not support sampler anisotropy");
        return FALSE;
    }

    return TRUE;
}

b8 vulkan_device_detect_depth_format(vulkan_device *device) {
    const u64 candidate_count = 3;
    VkFormat candidates[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for (u32 i = 0; i < candidate_count; ++i) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device->physical, candidates[i], &props);

        if ((props.linearTilingFeatures & flags) == flags) {
            device->depth_format = candidates[i];
            return TRUE;
        } else if ((props.optimalTilingFeatures & flags) == flags) {
            device->depth_format = candidates[i];
            return TRUE;
        }
    }
    return FALSE;
}
