#version 450

layout(set = 1, binding = 0) uniform UBO {
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 transform; // Model matrix
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPosition; // world-space
layout(location = 1) out vec3 fragNormal; // world-space
layout(location = 2) out vec2 fragTexCoord;

void main() {
    // World space position
    vec3 worldPos = (pushConstants.transform * vec4(inPosition, 1.0)).xyz;
    fragPosition = worldPos;

    // Pass texture coordinates
    fragTexCoord = inTexCoord;

    vec3 N = normalize(mat3(pushConstants.transform) * inNormal);

    fragNormal = N;

    // Clip-space position
    gl_Position = ubo.proj * ubo.view * vec4(worldPos, 1.0);
}
