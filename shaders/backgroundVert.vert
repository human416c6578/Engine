#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 localPos;

void main() {
    localPos = inPosition;
    mat4 rotView = mat4(mat3(ubo.view));
    vec4 clipPos = ubo.proj * rotView * vec4(localPos, 1.0);

	gl_Position = clipPos.xyww;
}
