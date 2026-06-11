#version 450

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D colorTex;
layout(binding = 1) uniform sampler2D depthTex;

layout(binding = 2) uniform PostProcessSettings
{
	// DOF
	float focusDistance;
	float focusRange;
	float blurScale;
	float maxBlur;

	// Fog
	int enableFog;
	float fogStart;
	float fogEnd;
	float fogIntensity;

	vec3 fogColor;

	// Tonemap
	int enableTonemap;
	float exposure;
} pp;

float linearizeDepth(float depth)
{
	float near = 0.01;
	float far  = 1000.0;

	float z = depth * 2.0 - 1.0;

	return (2.0 * near * far) /
		   (far + near - z * (far - near));
}

float hash(vec2 p)
{
	return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

vec3 tonemapACES(vec3 x)
{
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;

	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

float computeFogFactor(float depthLinear)
{
    float dist = max(depthLinear - pp.fogStart, 0.0);
    float fog = 1.0 - exp(-dist * pp.fogIntensity);
    return clamp(fog, 0.0, 1.0);
}

void main()
{
	vec2 texelSize = 1.0 / textureSize(colorTex, 0);

	vec3 sharpColor = texture(colorTex, inUV).rgb;

	float depth = texture(depthTex, inUV).r;
	float linearDepth = linearizeDepth(depth);

	float coc = abs(linearDepth - pp.focusDistance);

	float blurAmount = coc / max(pp.focusRange, 0.0001);

	blurAmount = clamp(blurAmount, 0.0, pp.maxBlur);

	blurAmount = blurAmount * blurAmount;

	float radius = blurAmount * pp.blurScale;

	vec3 blurColor = vec3(0.0);
	float total = 0.0;

	for (int x = -2; x <= 2; x++)
	{
		for (int y = -2; y <= 2; y++)
		{
			vec2 offset = vec2(x, y);

			float j = hash(inUV + offset) - 0.5;
			offset += j;

			vec2 uv = inUV + offset * texelSize * radius;

			blurColor += texture(colorTex, uv).rgb;
			total += 1.0;
		}
	}

	blurColor /= total;

	vec3 finalColor = mix(sharpColor, blurColor, blurAmount);

	if (pp.enableFog != 0)
	{
		float fogFactor = computeFogFactor(linearDepth);

		float skyMask = smoothstep(0.995, 1.0, linearDepth);

		if (pp.enableFog != 0)
		{
			float fogFactor = computeFogFactor(linearDepth);

			vec3 fogColor = pp.fogColor;

			finalColor = mix(finalColor, fogColor, fogFactor);
		}

		vec3 fogColor = pp.fogColor;

		finalColor = mix(finalColor, pp.fogColor, fogFactor);
	}

	if(pp.enableTonemap != 0)
		finalColor = tonemapACES(finalColor) * pp.exposure;

	outColor = vec4(finalColor, 1.0);
}