#version 450

layout(location = 0) out vec4 outColor;
layout(location = 1) out float outLifeFraction;

struct Particle
{
    vec4 position;
    vec4 velocity;

    vec4 colorStart;
    vec4 colorEnd;

    float sizeStart;
    float sizeEnd;

    uint emitterID;
    float _pad;
};

layout(std430, binding = 0) readonly buffer ParticleBuffer
{
    Particle particles[];
};

layout(push_constant) uniform Push
{
    mat4 projView;
} pushData;

void main()
{
    Particle p = particles[gl_VertexIndex];

    float lifeRatio =
        clamp(p.position.w / p.velocity.w, 0.0, 1.0);

    vec4 color = mix(
            p.colorEnd,
            p.colorStart,
            lifeRatio);

    float size = mix(
            p.sizeEnd,
            p.sizeStart,
            lifeRatio) * 0.1f;

    outColor = color;
    outLifeFraction = lifeRatio;

    vec4 clipPos =
        pushData.projView *
            vec4(p.position.xyz, 1.0);

    gl_Position = clipPos;

    float distance = abs(clipPos.w);

    float speed = length(p.velocity.xyz) * 0.01f;

    float sizeScale = 20.0;

    gl_PointSize = p.sizeStart * (sizeScale / distance);
}
