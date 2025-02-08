#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 transform;
    vec3 color;
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;

void main() {
    gl_Position = ubo.proj * ubo.view * pushConstants.transform * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
    fragNormal = inNormal;
    fragPosition = vec3(pushConstants.transform * vec4(inPosition, 1.0));
}
