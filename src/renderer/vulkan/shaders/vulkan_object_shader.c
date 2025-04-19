#include "vulkan_object_shader.h"

#include "core/cmemory.h"
#include "core/logger.h"
#include "math/math_types.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "renderer/vulkan/vulkan_shader_utils.h"

// Built-in shader name (not configurable)
#define BUILTIN_SHADER_NAME_OBJECT "Builtin.ObjectShader"

b8 vulkan_object_shader_create(vulkan_context *context, vulkan_object_shader *out_shader) {
    char stage_type_strs[OBJECT_SHADER_STAGE_COUNT][5] = {"vert", "frag"};
    VkShaderStageFlagBits stage_types[OBJECT_SHADER_STAGE_COUNT] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

    // iterage over the shader stages
    for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
        if (!create_shader_module(context, BUILTIN_SHADER_NAME_OBJECT, stage_type_strs[i], stage_types[i], i, out_shader->stages)) {
            LOG_ERROR("Failed to create shader module for stage %s", stage_type_strs[i]);
            return false;
        }
    }

    // TODO:  descriptors


    // pipeline creation
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = (f32)context->framebuffer_height;
    viewport.width = (f32)context->framebuffer_width;
    viewport.height = -(f32)context->framebuffer_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = context->framebuffer_width;
    scissor.extent.height = context->framebuffer_height;

    // attributes
    u32 offset = 0;
    const i32 attributes_count = 1;
    VkVertexInputAttributeDescription attribute_descriptions[1];
    // position
    VkFormat formats[1] = {
        VK_FORMAT_R32G32B32_SFLOAT, // even if this a format for color (rgb), it is used for position. but in vulkan,
                                    // we talk only with colors. So here position is described with three 32-bit floats
    };
    u64 sizes[1] = {
        sizeof(vec3)
    };

    for (i32 i = 0; i < attributes_count; ++i) {
        attribute_descriptions[i].binding = 0; // binding index - should match binding desc
        attribute_descriptions[i].location = i; // location of the attribute in the shader (location = 0 for ex)
        attribute_descriptions[i].format = formats[i];
        attribute_descriptions[i].offset = offset;
        offset += sizes[i];
    }

    // stages
    VkPipelineShaderStageCreateInfo stage_create_infos[OBJECT_SHADER_STAGE_COUNT];
    czero_memory(stage_create_infos, sizeof(stage_create_infos));
    for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
        stage_create_infos[i].sType = out_shader->stages[i].shader_stage_create_info.sType;
        stage_create_infos[i] = out_shader->stages[i].shader_stage_create_info;
    }

    if (!vulkan_graphics_pipeline_create(
            context,
            &context->main_renderpass,
            attributes_count,
            attribute_descriptions,
            0,
            0,
            OBJECT_SHADER_STAGE_COUNT,
            stage_create_infos,
            viewport,
            scissor,
            false,
            &out_shader->pipeline)) {
        LOG_ERROR("Failed to create graphics pipeline for object shader");
        return false;
    }


    return true;
}

void vulkan_object_shader_destroy(vulkan_context *context, struct vulkan_object_shader *shader) {
    // destroy pipeline
    vulkan_graphics_pipeline_destroy(context, &shader->pipeline);

    // destroy shader modules
    for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
        vkDestroyShaderModule(context->device.logical, shader->stages[i].handle, context->allocator);
        shader->stages[i].handle = 0;
    }
}

void vulkan_object_shader_use(vulkan_context *context, struct vulkan_object_shader *shader) {
    u32 image_index = context->image_index;
    vulkan_pipeline_bind(&context->graphics_command_buffers[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, &shader->pipeline);
}
