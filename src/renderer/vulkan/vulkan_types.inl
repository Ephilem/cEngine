#pragma once

#include "define.h"

#include <vulkan/vulkan.h>
#include "core/asserts.h"
#include "renderer/renderer_types.inl"

#define VK_CHECK(expr) \
    { \
        cASSERT(expr == VK_SUCCESS); \
    }

typedef struct vulkan_buffer {
    u64 total_size;
    VkBuffer handle;
    VkBufferUsageFlagBits usage;
    b8 is_locked;
    VkDeviceMemory memory;
    i32 memory_index;
    u32 mmeory_property_flags;
} vulkan_buffer;

typedef struct vulkan_swapchain_support_info {
    VkSurfaceCapabilitiesKHR capabilities;
    u32 format_count;
    VkSurfaceFormatKHR* formats;
    u32 present_mode_count;
    VkPresentModeKHR* present_modes;
} vulkan_swapchain_support_info;

typedef struct vulkan_device {
    VkPhysicalDevice physical;
    VkDevice logical;
    vulkan_swapchain_support_info swapchain_support_info;

    i32 graphics_queue_index;
    i32 present_queue_index;
    i32 compute_queue_index;
    i32 transfer_queue_index;

    VkQueue graphics_queue;
    VkQueue present_queue;
    VkQueue compute_queue;
    VkQueue transfer_queue;

    VkCommandPool graphics_command_pool;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;

    VkFormat depth_format;
} vulkan_device;

typedef struct vulkan_image {
    VkImage handle;
    VkDeviceMemory memory; // memory allocated in the gpu for the image
    VkImageView view;
    u32 width;
    u32 height;
} vulkan_image;


typedef enum vulkan_render_pass_state {
    READY,
    RECORDING,
    IN_RENDER_PASS,
    RECORDING_ENDED,
    SUBMITTED,
    NOT_ALLOCATED,
} vulkan_render_pass_state;

typedef struct vulkan_renderpass {
    VkRenderPass handle;
    f32 x, y, w, h;
    f32 r, g, b, a;

    f32 depth;
    u32 stencil;

    vulkan_render_pass_state state;
} vulkan_renderpass;

typedef struct vulkan_framebuffer {
    VkFramebuffer handle;
    u32 attachment_count;
    VkImageView* attachments;
    vulkan_renderpass* renderpass;
} vulkan_framebuffer;

typedef struct vulkan_swapchain {
    VkSurfaceFormatKHR format;
    u8 max_frames_in_flight;

    VkSwapchainKHR handle;
    VkExtent2D extent;

    u32 image_count;
    VkImage* images;
    VkImageView* views;

    vulkan_image depth_attachment;

    // famebuffers used for on-screen rendering
    vulkan_framebuffer* framebuffers;
} vulkan_swapchain;

typedef enum vulkan_command_buffer_state {
    COMMAND_BUFFER_STATE_READY,
    COMMAND_BUFFER_STATE_RECORDING,
    COMMAND_BUFFER_STATE_IN_RENDER_PASS,
    COMMAND_BUFFER_STATE_RECORDING_ENDED,
    COMMAND_BUFFER_STATE_SUBMITTED,
    COMMAND_BUFFER_STATE_NOT_ALLOCATED,
} vulkan_command_buffer_state;

typedef struct vulkan_command_buffer {
    VkCommandBuffer handle;

    vulkan_command_buffer_state state;
} vulkan_command_buffer;

typedef struct vulkan_fence {
    VkFence handle;
    b8 is_signaled;
} vulkan_fence;

typedef struct vulkan_shader_stage {
    VkShaderModuleCreateInfo create_info;
    VkShaderModule handle;
    VkPipelineShaderStageCreateInfo shader_stage_create_info;
} vulkan_shader_stage;

typedef struct vulkan_pipeline {
    VkPipeline handle;
    VkPipelineLayout pipeline_layout;
} vulkan_pipeline;

#define OBJECT_SHADER_STAGE_COUNT 2
typedef struct vulkan_object_shader {
    // vertex & fragment shader
    vulkan_shader_stage stages[OBJECT_SHADER_STAGE_COUNT];

    vulkan_pipeline pipeline;

    VkDescriptorPool global_descriptor_pool;
    VkDescriptorSetLayout global_descriptor_set_layout; // define the layout of the descriptor set
    VkDescriptorSet global_descriptor_sets[4]; // One descriptor set per frame
    b8 global_descriptor_updated[4];

    global_uniform_object global_ubo;
    vulkan_buffer global_uniform_buffer; // linked to the descriptor set
} vulkan_object_shader;

typedef struct vulkan_context {
    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;

    u32 framebuffer_width;
    u32 framebuffer_height;

    u64 framebuffer_size_generation;
    u64 framebuffer_size_last_generation;

#if _DEBUG
    VkDebugUtilsMessengerEXT debug_messenger;
#endif

    vulkan_device device;
    b8 recreating_swapchain;

    vulkan_swapchain swapchain;
    vulkan_renderpass main_renderpass; // the main renderpass for the application

    // buffers
    vulkan_buffer object_vertex_buffer;
    vulkan_buffer object_index_buffer;

    u64 geometry_vertex_offset;
    u64 geometry_index_offset;

    // darray
    vulkan_command_buffer* graphics_command_buffers; // commands buffer available for the graphics queue (one per frame)

    // darray
    VkSemaphore* image_available_semaphores; // triggered when the image is available (after is done presenting)

    // darray
    VkSemaphore* queue_complete_semaphores; // triggered when a queue is done with the image

    u32 in_flight_fence_count;
    vulkan_fence* in_flight_fences;

    // holds pointers to fences which exist and are owned elsewhere
    vulkan_fence** images_in_flight;

    u32 image_index;
    u32 current_frame;

    i32 (*find_memory_index)(u32 type_filter, u32 property_flags); // function to find the memory index for a given type filter and property flags


    // Shaders
    vulkan_object_shader object_shader;


} vulkan_context;

