#include "vulkan_shader_utils.h"

#include "core/cstring.h"
#include "core/logger.h"
#include "platform/filesystem.h"
#include "core/cmemory.h"

#define SHADERS_PATH "shaders"




b8 create_shader_module(vulkan_context *context, const char *shader_name, const char *stage_type_str,
    VkShaderStageFlagBits shader_stage_flag, u32 stage_index, vulkan_shader_stage *shader_stages) {

    // Build file name :
    char file_name[1024];
    // TODO: use a resource system instead of the hack of ../ to get the cEngine buildin compiled shaders folder
    char relative_path[1024];
    string_format(relative_path, "../%s/%s.%s.spv", SHADERS_PATH, shader_name, stage_type_str);

    char executable_dir[1024];
    filesystem_get_executable_dir(executable_dir);
    string_format(file_name, "%s/%s", executable_dir, relative_path);

    LOG_DEBUG("Loading shader from: %s", file_name);
    LOG_DEBUG("Shader file exists: %s", filesystem_exists(file_name) ? "YES" : "NO");


    czero_memory(&shader_stages[stage_index].create_info, sizeof(VkShaderModuleCreateInfo));
    shader_stages[stage_index].create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;


    // obtain file handler
    file_handle handle;
    if (!filesystem_open(file_name, FILE_MODE_READ, true, &handle)) {
        LOG_ERROR("Failed to open shader file %s", file_name);
        return false;
    }

    // read the file
    u64 file_size = 0;
    u8* file_buffer = 0;
    if (!filesystem_read_all_bytes(&handle, &file_buffer, &file_size)) {
        LOG_ERROR("Failed to read shader file %s", file_name);
        filesystem_close(&handle);
        return false;
    }
    shader_stages[stage_index].create_info.codeSize = file_size;
    shader_stages[stage_index].create_info.pCode = (u32*)file_buffer;

    filesystem_close(&handle);

    VK_CHECK(vkCreateShaderModule(
        context->device.logical,
        &shader_stages[stage_index].create_info,
        context->allocator,
        &shader_stages[stage_index].handle));

    czero_memory(&shader_stages[stage_index].shader_stage_create_info, sizeof(VkShaderModuleCreateInfo));
    shader_stages[stage_index].shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[stage_index].shader_stage_create_info.stage = shader_stage_flag;
    shader_stages[stage_index].shader_stage_create_info.module = shader_stages[stage_index].handle;
    shader_stages[stage_index].shader_stage_create_info.pName = "main"; // entry point name

    if (file_buffer) {
        cfree(file_buffer, sizeof(u8) * file_size, MEMORY_TAG_STRING);
        file_buffer = 0;
    }

    return true;
}
