#version 450

layout(set = 1, binding = 0) uniform UBO {
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 transform;  // Model matrix
    vec3 color;      // Color
} pushConstants;

layout(location = 0) in vec3 inPosition;   // Vertex position
layout(location = 1) in vec3 inNormal;     // Vertex normal
layout(location = 2) in vec2 inTexCoord;   // Vertex texture coordinates

layout(location = 0) out vec3 fragPosition;  // Fragment position (world space)
layout(location = 1) out vec2 fragTexCoord;  // Fragment texture coordinates
layout(location = 2) out vec3 fragNormal;    // Fragment normal (world space)

void main() {
    // Transform position using model, view, and projection matrices
    vec4 worldPosition = pushConstants.transform * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPosition;

    // Pass texture coordinates and normal
    fragTexCoord = inTexCoord;
    fragNormal = normalize(mat3(pushConstants.transform) * inNormal);  // Transform normal

    // Pass the world space position to the fragment shader
    fragPosition = worldPosition.xyz;
}
