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


layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

vec3 lightColors[4] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    vec3(0.0, 1.0, 1.0)
);

vec3 lightPositions[4] = vec3[](
    vec3(0.0, -2.0, -1.0),
    vec3(1.0, -2.0, -1.0),
    vec3(-1.0, -2.0, -1.0),
    vec3(2.0, -2.0, -1.0)
);

void main()
{		
    outColor = vec4(pushConstants.color, 1.0);
}