#pragma once

#include "renderer/vulkan/vulkan_types.inl"
#include "renderer/renderer_types.inl"


b8 vulkan_material_shader_create(vulkan_context* context, vulkan_material_shader* out_shader);

void vulkan_material_shader_destroy(vulkan_context* context, struct vulkan_material_shader* shader);

void vulkan_material_shader_use(vulkan_context* context, struct vulkan_material_shader* shader);

void vulkan_material_shader_update_global_state(vulkan_context* context, struct vulkan_material_shader* shader, f32 delta_time);

/**
 * Setting the model matrix into the buffer
 */
void vulkan_material_shader_set_model(vulkan_context* context, struct vulkan_material_shader* shader, mat4 model);
void vulkan_material_shader_apply_material(vulkan_context* context, struct vulkan_material_shader* shader, material* material);

/**
 * Prepare a material to be able to be used with a shader.
 * @param context Vulkan context
 * @param shader The shader instance to configure
 * @param material the material to acquire resources for
 * @return true if the material was successfully acquired, false otherwise
 */
b8 vulkan_material_shader_acquire_resources(vulkan_context* context, struct vulkan_material_shader* shader, material* material);
void vulkan_material_shader_release_resources(vulkan_context* context, struct vulkan_material_shader* shader, material* material);