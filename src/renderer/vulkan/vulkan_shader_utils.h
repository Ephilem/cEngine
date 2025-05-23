#pragma once

#include "vulkan_types.inl"

b8 create_shader_module(
    vulkan_context* context,
    const char* shader_name,
    const char* stage_type_str,
    VkShaderStageFlagBits shader_stage_flag,
    u32 stage_index,
    vulkan_shader_stage* shader_stages);
