#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} ubo;

layout(location = 0) in vec3 localPos;
layout(location = 0) out vec4 outColor;
layout(set = 0, binding = 1) uniform samplerCube environmentMap;

void main() {	
    vec3 envColor = texture(environmentMap, localPos).rgb;
    envColor = envColor / (envColor + vec3(1.0)); 
    //envColor = pow(envColor, vec3(1.0 / 2.2)); 
    //gl_FragDepth = 0.99999;
    outColor = vec4(envColor, 1.0);
}
