#version 450

layout(set = 1, binding = 0) uniform UBO {
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 transform;  // Model matrix
} pushConstants;

layout(location = 0) in vec3 inPosition;   // Vertex position
layout(location = 1) in vec3 inNormal;     // Vertex normal
layout(location = 2) in vec2 inTexCoord;   // Vertex texture coordinates

layout(location = 0) out vec3 fragPosition;  // Fragment position (world space)
layout(location = 1) out vec2 fragTexCoord;  // Texture coordinates
layout(location = 2) out vec3 fragNormal;    // Normal (world space)
layout(location = 3) out vec3 viewDir;       // View direction (world space)

void main() {
    // World space position
    vec4 worldPosition = pushConstants.transform * vec4(inPosition, 1.0);
    fragPosition = worldPosition.xyz;

    // Texture coordinates passthrough
    fragTexCoord = inTexCoord;

    // Normal in world space
    mat3 normalMatrix = transpose(inverse(mat3(pushConstants.transform)));
    fragNormal = normalize(normalMatrix * inNormal);

    // Final position in clip space
    gl_Position = ubo.proj * ubo.view * worldPosition;
}
