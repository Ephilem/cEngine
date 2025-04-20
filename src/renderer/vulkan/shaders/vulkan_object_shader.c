#include "vulkan_object_shader.h"

#include "core/cmemory.h"
#include "core/logger.h"
#include "math/math_types.h"
#include "renderer/vulkan/vulkan_buffer.h"
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

    // Global descriptors
    VkDescriptorSetLayoutBinding global_ubo_layout_binding;
    global_ubo_layout_binding.binding = 0;
    global_ubo_layout_binding.descriptorCount = 1;
    global_ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    global_ubo_layout_binding.pImmutableSamplers = 0;
    global_ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // at what stage this descriptor will be provided

    VkDescriptorSetLayoutCreateInfo global_layout_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    global_layout_info.bindingCount = 1;
    global_layout_info.pBindings = &global_ubo_layout_binding;
    VK_CHECK(vkCreateDescriptorSetLayout(context->device.logical, &global_layout_info, context->allocator, &out_shader->global_descriptor_set_layout));

    VkDescriptorPoolSize global_pool_size;
    global_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    global_pool_size.descriptorCount = context->swapchain.image_count;

    VkDescriptorPoolCreateInfo global_pool_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    global_pool_info.poolSizeCount = 1;
    global_pool_info.pPoolSizes = &global_pool_size;
    global_pool_info.maxSets = context->swapchain.image_count;

    VK_CHECK(vkCreateDescriptorPool(context->device.logical, &global_pool_info, context->allocator, &out_shader->global_descriptor_pool));

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

    // Descriptor set layouts
    const i32 descriptor_set_layout_count = 1;
    VkDescriptorSetLayout layouts[1] = {
        out_shader->global_descriptor_set_layout
    };


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
            descriptor_set_layout_count,
            layouts,
            OBJECT_SHADER_STAGE_COUNT,
            stage_create_infos,
            viewport,
            scissor,
            false,
            &out_shader->pipeline)) {
        LOG_ERROR("Failed to create graphics pipeline for object shader");
        return false;
    }

    // create the uniform buffer
    if (!vulkan_buffer_create(
            context,
            sizeof(global_uniform_object) * 4,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, // transfer destination because we will copy data to it | declared as uniform buffer
            // local memory in the device       | can be accessed by the host (CPU) | coherent memory (no need to flush)
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            true,
            &out_shader->global_uniform_buffer)) {
        LOG_ERROR("Failed to create global uniform buffer");
        return false;
    }

    VkDescriptorSetLayout global_layouts[4] = {
        out_shader->global_descriptor_set_layout,
        out_shader->global_descriptor_set_layout,
        out_shader->global_descriptor_set_layout,
        out_shader->global_descriptor_set_layout
    };

    VkDescriptorSetAllocateInfo allocate_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocate_info.descriptorPool = out_shader->global_descriptor_pool;
    allocate_info.descriptorSetCount = context->swapchain.image_count;
    allocate_info.pSetLayouts = global_layouts;
    VK_CHECK(vkAllocateDescriptorSets(context->device.logical, &allocate_info, out_shader->global_descriptor_sets));


    return true;
}

void vulkan_object_shader_destroy(vulkan_context *context, struct vulkan_object_shader *shader) {
    VkDevice logical = context->device.logical;

    vulkan_buffer_destroy(context, &shader->global_uniform_buffer);

    vulkan_graphics_pipeline_destroy(context, &shader->pipeline);

    vkDestroyDescriptorPool(logical, shader->global_descriptor_pool, context->allocator);
    vkDestroyDescriptorSetLayout(logical, shader->global_descriptor_set_layout, context->allocator);

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

void vulkan_object_shader_update_global_state(vulkan_context *context, struct vulkan_object_shader *shader) {
    u32 image_index = context->image_index;
    VkCommandBuffer command_buffer = context->graphics_command_buffers[image_index].handle;
    VkDescriptorSet global_descriptor = shader->global_descriptor_sets[image_index];

    // config the descriptors for the given index
    if (!shader->global_descriptor_updated[image_index]) {
        u32 range = sizeof(global_uniform_object);
        u64 offset = sizeof(global_uniform_object) * image_index;

        vulkan_buffer_load_data(context, &shader->global_uniform_buffer, offset, range, 0, &shader->global_ubo);

        // signal the gpu to update the descriptor set
        VkDescriptorBufferInfo buffer_info;
        buffer_info.buffer = shader->global_uniform_buffer.handle;
        buffer_info.offset = offset;
        buffer_info.range = range;

        VkWriteDescriptorSet descriptor_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        descriptor_write.dstSet = shader->global_descriptor_sets[image_index];
        descriptor_write.dstBinding = 0;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pBufferInfo = &buffer_info;

        vkUpdateDescriptorSets(context->device.logical, 1, &descriptor_write, 0, 0);
        shader->global_descriptor_updated[image_index] = true;
    }

    // bind the global descriptor set to be updated
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->pipeline.pipeline_layout, 0, 1, &global_descriptor, 0, 0);


}

void vulkan_object_shader_update_object(vulkan_context *context, struct vulkan_object_shader* shader, mat4 model) {
    u32 image_index = context->image_index;
    VkCommandBuffer command_buffer = context->graphics_command_buffers[image_index].handle;

    vkCmdPushConstants(command_buffer, shader->pipeline.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &model);
}
