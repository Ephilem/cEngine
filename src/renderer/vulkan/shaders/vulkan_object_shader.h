#pragma once

#include "renderer/vulkan/vulkan_types.inl"
#include "renderer/renderer_types.inl"

/**
 * Create a a vulkan object shader (code used in the gpu)
 * @param context vulkan context
 * @param out_shader created shader
 * @return
 */
b8 vulkan_object_shader_create(vulkan_context* context, vulkan_object_shader* out_shader);

/**
 * Create a vulkan object shader from a file
 * @param context vulkan context
 * @param out_shader created shader
 * @param filename path to the shader file
 * @return
 */
void vulkan_object_shader_destroy(vulkan_context* context, struct vulkan_object_shader* shader);

void vulkan_object_shader_use(vulkan_context* context, struct vulkan_object_shader* shader);

void vulkan_object_shader_update_global_state(vulkan_context* context, struct vulkan_object_shader* shader);

void vulkan_object_shader_update_object(vulkan_context* context, struct vulkan_object_shader* shader, mat4 model);