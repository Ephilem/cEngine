#include "vulkan_shader_utils.h"

#include "core/cstring.h"
#include "core/logger.h"
#include "core/cmemory.h"
#include "systems/resource_system.h"


b8 create_shader_module(vulkan_context *context, const char *shader_name, const char *stage_type_str,
                        VkShaderStageFlagBits shader_stage_flag, u32 stage_index, vulkan_shader_stage *shader_stages) {
    char file_name[1024];
    string_format(file_name, "shaders/%s.%s.spv", shader_name, stage_type_str);

    czero_memory(&shader_stages[stage_index].create_info, sizeof(VkShaderModuleCreateInfo));
    shader_stages[stage_index].create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    // read the resource
    resource binary_resource;
    if (!resource_system_load(file_name, RESOURCE_TYPE_BINARY, &binary_resource)) {
        LOG_ERROR("Failed to load shader file %s", file_name);
        return false;
    }

    shader_stages[stage_index].create_info.codeSize = binary_resource.data_size;
    shader_stages[stage_index].create_info.pCode = (u32*)binary_resource.data;

    VK_CHECK(vkCreateShaderModule(
        context->device.logical,
        &shader_stages[stage_index].create_info,
        context->allocator,
        &shader_stages[stage_index].handle));

    // release the resource
    resource_system_unload(&binary_resource);

    czero_memory(&shader_stages[stage_index].shader_stage_create_info, sizeof(VkShaderModuleCreateInfo));
    shader_stages[stage_index].shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[stage_index].shader_stage_create_info.stage = shader_stage_flag;
    shader_stages[stage_index].shader_stage_create_info.module = shader_stages[stage_index].handle;
    shader_stages[stage_index].shader_stage_create_info.pName = "main"; // entry point name

    return true;
}
