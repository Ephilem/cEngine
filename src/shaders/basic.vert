#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform PushConstants {
    mat4 model;
} pushConstants;

layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 projection;
} ubo;

void main() {
    gl_Position = ubo.projection * ubo.view * pushConstants.model * vec4(inPosition, 1.0);
    fragColor = inColor;
}
