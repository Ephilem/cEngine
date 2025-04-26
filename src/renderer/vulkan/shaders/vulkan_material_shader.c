#include "vulkan_material_shader.h"

#include "core/cmemory.h"
#include "core/logger.h"
#include "math/math_types.h"
#include "renderer/vulkan/vulkan_buffer.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "renderer/vulkan/vulkan_shader_utils.h"
#include "math/cmath.h"
#include "systems/texture_system.h"

// Built-in shader name (not configurable)
#define BUILTIN_SHADER_NAME_MATERIAL "Builtin.MaterialShader"

b8 vulkan_material_shader_create(vulkan_context *context, vulkan_material_shader *out_shader) {
    char stage_type_strs[MATERIAL_SHADER_STAGE_COUNT][5] = {"vert", "frag"};
    VkShaderStageFlagBits stage_types[MATERIAL_SHADER_STAGE_COUNT] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

    // iterage over the shader stages
    for (u32 i = 0; i < MATERIAL_SHADER_STAGE_COUNT; ++i) {
        if (!create_shader_module(context, BUILTIN_SHADER_NAME_MATERIAL, stage_type_strs[i], stage_types[i], i, out_shader->stages)) {
            LOG_ERROR("Failed to create shader module for stage %s", stage_type_strs[i]);
            return false;
        }
    }

    ////// GLOBAL DESCRIPTOR POOL //////
    //// For data like projection matrix or view matrix
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

    out_shader->sampler_uses[0] = TEXTURE_USE_MAP_DIFFUSE;
    ////// LOCAL/OBJECT DESCRIPTOR POOL //////
    //// For data only for this "object" (like diffuse color, textures, etc...)
    const u32 local_sampler_count = 1; // number of sampler for each objects
    VkDescriptorType descriptor_types[VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT] = {
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // binding 0 - uniform buffer
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER // binding 1 - diffuse sample layout.
    };
    VkDescriptorSetLayoutBinding bindings[VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT];
    czero_memory(bindings, sizeof(bindings));
    for (u32 i = 0; i < VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT; ++i) {
        bindings[i].binding = i;
        bindings[i].descriptorCount = 1;
        bindings[i].descriptorType = descriptor_types[i];
        bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    VkDescriptorSetLayoutCreateInfo layout_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layout_info.bindingCount = VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT;
    layout_info.pBindings = bindings;
    VK_CHECK(vkCreateDescriptorSetLayout(context->device.logical, &layout_info, context->allocator, &out_shader->object_descriptor_set_layout));

    VkDescriptorPoolSize object_pool_sizes[2];
    object_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    object_pool_sizes[0].descriptorCount = VULKAN_MAX_MATERIAL_COUNT;
    object_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    object_pool_sizes[1].descriptorCount = VULKAN_MATERIAL_SHADER_SAMPLER_COUNT * VULKAN_MAX_MATERIAL_COUNT;

    VkDescriptorPoolCreateInfo object_pool_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    object_pool_info.poolSizeCount = 2;
    object_pool_info.pPoolSizes = object_pool_sizes;
    object_pool_info.maxSets = VULKAN_MAX_MATERIAL_COUNT;
    object_pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // explicitly free descriptor sets
    VK_CHECK(vkCreateDescriptorPool(context->device.logical, &object_pool_info, context->allocator, &out_shader->object_descriptor_pool));

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

    //// attributes
    u32 offset = 0;
#define ATTRIBUTE_COUNT 2 // position + uv's texcoord
    VkVertexInputAttributeDescription attribute_descriptions[ATTRIBUTE_COUNT];
    // Position, texcoord
    VkFormat formats[ATTRIBUTE_COUNT] = {
        VK_FORMAT_R32G32B32_SFLOAT, // even if this a format for color (rgb), it is used for position. but in vulkan,
                                    // we talk only with colors. So here position is described with three 32-bit floats
        VK_FORMAT_R32G32_SFLOAT,
    };
    u64 sizes[ATTRIBUTE_COUNT] = {
        sizeof(vec3),
        sizeof(vec2),
    };

    for (i32 i = 0; i < ATTRIBUTE_COUNT; ++i) {
        attribute_descriptions[i].binding = 0; // binding index - should match binding desc
        attribute_descriptions[i].location = i; // location of the attribute in the shader (location = 0 for ex)
        attribute_descriptions[i].format = formats[i];
        attribute_descriptions[i].offset = offset;
        offset += sizes[i];
    }

    // Descriptor set layouts
    const i32 descriptor_set_layout_count = 2; // global + object
    VkDescriptorSetLayout layouts[2] = {
        out_shader->global_descriptor_set_layout,
        out_shader->object_descriptor_set_layout
    };


    // stages
    VkPipelineShaderStageCreateInfo stage_create_infos[MATERIAL_SHADER_STAGE_COUNT];
    czero_memory(stage_create_infos, sizeof(stage_create_infos));
    for (u32 i = 0; i < MATERIAL_SHADER_STAGE_COUNT; ++i) {
        stage_create_infos[i].sType = out_shader->stages[i].shader_stage_create_info.sType;
        stage_create_infos[i] = out_shader->stages[i].shader_stage_create_info;
    }

    if (!vulkan_graphics_pipeline_create(
            context,
            &context->main_renderpass,
            ATTRIBUTE_COUNT,
            attribute_descriptions,
            descriptor_set_layout_count,
            layouts,
            MATERIAL_SHADER_STAGE_COUNT,
            stage_create_infos,
            viewport,
            scissor,
            false,
            &out_shader->pipeline)) {
        LOG_ERROR("Failed to create graphics pipeline for object shader");
        return false;
    }

    // create the global uniform
    u32 device_local_bits = context->device.supports_device_local_host_visible ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0;
    if (!vulkan_buffer_create(
            context,
            sizeof(global_uniform_object) * 4,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            device_local_bits | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            true,
            &out_shader->global_uniform_buffer)) {
        LOG_ERROR("Failed to create global uniform buffer");
        return false;
    }

    // Allocate for the descriptor sets of the global uniform buffer
    VkDescriptorSetLayout global_layouts[4] = {
        out_shader->global_descriptor_set_layout,
        out_shader->global_descriptor_set_layout,
        out_shader->global_descriptor_set_layout,
        out_shader->global_descriptor_set_layout
    };

    VkDescriptorSetAllocateInfo allocate_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocate_info.descriptorPool = out_shader->global_descriptor_pool;
    allocate_info.descriptorSetCount = 4;
    allocate_info.pSetLayouts = global_layouts;
    VK_CHECK(vkAllocateDescriptorSets(context->device.logical, &allocate_info, out_shader->global_descriptor_sets));

    // create the object uniform buffer
    if (!vulkan_buffer_create(
        context,
        sizeof(material_uniform_object) * VULKAN_MAX_MATERIAL_COUNT,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        true,
        &out_shader->object_uniform_buffer)) {
        LOG_ERROR("Failed to create object uniform buffer");
        return false;
    }


    return true;
}

void vulkan_material_shader_destroy(vulkan_context *context, struct vulkan_material_shader *shader) {
    VkDevice logical = context->device.logical;

    vulkan_buffer_destroy(context, &shader->global_uniform_buffer);
    vulkan_buffer_destroy(context, &shader->object_uniform_buffer);

    vulkan_graphics_pipeline_destroy(context, &shader->pipeline);

    vkDestroyDescriptorPool(logical, shader->global_descriptor_pool, context->allocator);
    vkDestroyDescriptorSetLayout(logical, shader->global_descriptor_set_layout, context->allocator);

    vkDestroyDescriptorPool(logical, shader->object_descriptor_pool, context->allocator);
    vkDestroyDescriptorSetLayout(logical, shader->object_descriptor_set_layout, context->allocator);

    // destroy shader modules
    for (u32 i = 0; i < MATERIAL_SHADER_STAGE_COUNT; ++i) {
        vkDestroyShaderModule(context->device.logical, shader->stages[i].handle, context->allocator);
        shader->stages[i].handle = 0;
    }
}

void vulkan_material_shader_use(vulkan_context *context, struct vulkan_material_shader *shader) {
    u32 image_index = context->image_index;
    vulkan_pipeline_bind(&context->graphics_command_buffers[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, &shader->pipeline);
}

void vulkan_material_shader_update_global_state(vulkan_context *context, struct vulkan_material_shader *shader, f32 delta_time) {
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

void vulkan_material_shader_set_model(vulkan_context* context, struct vulkan_material_shader* shader, mat4 model) {
    if (context && shader) {
        u32 image_index = context->image_index;
        VkCommandBuffer command_buffer = context->graphics_command_buffers[image_index].handle;

        // We need to push the model matrix to the shader
        vkCmdPushConstants(command_buffer, shader->pipeline.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &model);
    }
}

void vulkan_material_shader_apply_material(vulkan_context *context, struct vulkan_material_shader* shader, material* material) {
    if (context && shader) {
        u32 image_index = context->image_index;
        VkCommandBuffer command_buffer = context->graphics_command_buffers[image_index].handle;

        // obtain material data
        vulkan_material_shader_instance_state* object_state = &shader->instance_states[material->internal_id];
        VkDescriptorSet object_descriptor_set = object_state->descriptor_sets[image_index];

        // TODO: if needs update, for now we always update
        VkWriteDescriptorSet descriptor_writes[VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT];
        czero_memory(descriptor_writes, sizeof(descriptor_writes));
        u32 descriptor_count = 0;
        u32 descriptor_index = 0;

        //////////////////////////////////////////////////////////////////
        // Descriptor 0 - Uniform buffer (constant data in a draw call)
        u32 range = sizeof(material_uniform_object);
        u64 offset = sizeof(material_uniform_object) * material->internal_id; // where we need to update
        material_uniform_object obo;

        /*
        static f32 accumulator = 0.0f;
        accumulator += context->frame_delta_time;
        f32 s = (c_sinf(accumulator) + 1.0f) / 2.0f; // scale from -1 to 1 to 0 to 1
        obo.diffuse_color = vec4_create(s, s, s, 1.0f);*/
        obo.diffuse_color = material->diffuse_color;

        // load the data into the buffer : writing the specific object data into the buffer so the shader can render with the right property
        vulkan_buffer_load_data(context, &shader->object_uniform_buffer, offset, range, 0, &obo);

        // check if the data is already loaded (using generation).
        // We do not need to do that after because the memory is already mapped
        u32* global_ubo_generation = &object_state->descriptor_states[descriptor_index].generations[image_index];
        if (*global_ubo_generation == INVALID_ID || *global_ubo_generation != material->generation) {
            VkDescriptorBufferInfo buffer_info;
            buffer_info.buffer = shader->object_uniform_buffer.handle;
            buffer_info.offset = offset;
            buffer_info.range = range;

            VkWriteDescriptorSet descriptor = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            descriptor.dstSet = object_descriptor_set;
            descriptor.dstBinding = descriptor_index;
            descriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor.descriptorCount = 1;
            descriptor.pBufferInfo = &buffer_info;

            descriptor_writes[descriptor_index] = descriptor;
            descriptor_count++;

            *global_ubo_generation = material->generation;
        }
        descriptor_index++;

        ///////////////////////////////////////
        // Descriptor 1 - Texture sampler
        const u32 sampler_count = 1;
        VkDescriptorImageInfo image_infos[1];
        for (u32 sampler_index = 0; sampler_index < sampler_count; ++sampler_index) {
            texture_use use = shader->sampler_uses[sampler_index];
            texture* t = 0;

            // get the texture from the material
            switch (use) {
                case TEXTURE_USE_MAP_DIFFUSE:
                    t = material->diffuse_map.texture;
                break;
                default:
                    LOG_FATAL("Unable to bind sample to unkown use.");
                return;
            }

            u32* descriptor_generation = &object_state->descriptor_states[descriptor_index].generations[image_index];
            u32* descriptor_id = &object_state->descriptor_states[descriptor_index].ids[image_index];

            // if the texture hasn't been loaded yet, just use the default texture
            if (!t || t->generation == INVALID_ID) {
                t = texture_system_get_default_texture();

                // reset the descriptor generation if using the default texture
                *descriptor_generation = INVALID_ID;
            }

            // check if we need to update the descriptor set
            // Check of : If the texture generation is different | if the descriptor_generation is invalid | if the descriptor id is different (texture changed)
            if (t && (*descriptor_generation != t->generation || *descriptor_generation == INVALID_ID || *descriptor_id != t->id)) {
                vulkan_texture_data* internal_data = (vulkan_texture_data*)t->internal_data;

                image_infos[sampler_index].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                image_infos[sampler_index].imageView = internal_data->image.view;
                image_infos[sampler_index].sampler = internal_data->sampler;

                VkWriteDescriptorSet descriptor = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
                descriptor.dstSet = object_descriptor_set;
                descriptor.dstBinding = descriptor_index;
                descriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptor.descriptorCount = 1;
                descriptor.pImageInfo = &image_infos[sampler_index];

                descriptor_writes[descriptor_count] = descriptor; // use descriptor_count instead of descriptor_index
                descriptor_count++;

                // sync frame generation if not using a default texture
                if (t->generation != INVALID_ID) {
                    *descriptor_generation = t->generation;
                    *descriptor_id = t->id;
                }
            }
            descriptor_index++;
        }

        if (descriptor_count > 0) {
            vkUpdateDescriptorSets(context->device.logical, descriptor_count, descriptor_writes, 0, 0);
        }

        // bind the descriptor set to be updated, or in case the shader changed
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->pipeline.pipeline_layout, 1, 1, &object_descriptor_set, 0, 0);
    }

}

b8 vulkan_material_shader_acquire_resources(vulkan_context *context, struct vulkan_material_shader *shader, material* material) {
    material->internal_id = shader->object_uniform_buffer_index;
    shader->object_uniform_buffer_index++;

    vulkan_material_shader_instance_state* object_state = &shader->instance_states[material->internal_id];
    for (u32 i = 0; i < VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT; ++i) {
        for (u32 j = 0; j < context->swapchain.image_count; ++j) {
            object_state->descriptor_states[i].generations[j] = INVALID_ID;
            object_state->descriptor_states[i].ids[j] = INVALID_ID;
        }
    }

    VkDescriptorSetLayout layouts[4] = {
        shader->object_descriptor_set_layout,
        shader->object_descriptor_set_layout,
        shader->object_descriptor_set_layout,
        shader->object_descriptor_set_layout
    };

    VkDescriptorSetAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc_info.descriptorPool = shader->object_descriptor_pool;
    alloc_info.descriptorSetCount = context->swapchain.image_count; // one descriptor set per frame
    alloc_info.pSetLayouts = layouts;
    VkResult result = vkAllocateDescriptorSets(context->device.logical, &alloc_info, object_state->descriptor_sets);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to allocate descriptor sets in shader!");
        return false;
    }

    return true;
}

void vulkan_material_shader_release_resources(vulkan_context *context, struct vulkan_material_shader *shader, material* material) {
    vulkan_material_shader_instance_state* instance_state = &shader->instance_states[material->internal_id];

    const u32 descriptor_set_count = 4;

    vkDeviceWaitIdle(context->device.logical);

    VkResult result = vkFreeDescriptorSets(context->device.logical, shader->object_descriptor_pool, descriptor_set_count, instance_state->descriptor_sets);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to free descriptor sets in shader!");
    }

    for (u32 i = 0; i < VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT; ++i) {
        for (u32 j = 0; j < context->swapchain.image_count; ++j) {
            instance_state->descriptor_states[i].generations[j] = INVALID_ID;
            instance_state->descriptor_states[i].ids[j] = INVALID_ID;
        }
    }

    material->internal_id = INVALID_ID;

    // todo: add the free object to a free list
}

