#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 out_colour;

// descriptor set 1, binding 0
layout(set = 1, binding = 0) uniform local_uniform_object {
    vec4 diffuse_color;
} object_ubo;

// descriptor set 1, binding 1
layout(set = 1, binding = 1) uniform sampler2D texture_diffuse;

// data transfer object
layout(location = 1) in struct dto {
    vec2 tex_coord;
} in_dto;

void main() {
    // texture() interpolates the texture coordinates using the sampler
    out_colour = object_ubo.diffuse_color * texture(texture_diffuse, in_dto.tex_coord);
}