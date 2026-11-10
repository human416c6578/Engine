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
    const float nearPlane = 0.1;
    const float farPlane = 100.0;

    return (nearPlane * farPlane) /
        (farPlane - depth * (farPlane - nearPlane));
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
    float range = max(pp.fogEnd - pp.fogStart, 0.0001);
    float distanceInFog = clamp(depthLinear - pp.fogStart, 0.0, range);
    float density = max(pp.fogIntensity, 0.0);
    // Fade smoothly across the authored range; intensity controls its strength.
    float rangeFade = smoothstep(0.0, range, distanceInFog);
    return rangeFade * (1.0 - exp(-distanceInFog * density));
}

float fetchDepth(vec2 uv)
{
    ivec2 size = textureSize(depthTex, 0);
    ivec2 pixel = clamp(ivec2(uv * vec2(size)), ivec2(0), size - ivec2(1));
    return texelFetch(depthTex, pixel, 0).r;
}

// Reconstruct bilinear color using only texels on the same side of the
// sky/geometry boundary. Checking just the sample center still lets the
// hardware's linear color filter pull in opposite-side neighbors.
vec3 sampleSurfaceColor(vec2 uv, bool isSky, vec3 fallbackColor)
{
    ivec2 size = textureSize(colorTex, 0);
    vec2 position = uv * vec2(size) - 0.5;
    ivec2 base = ivec2(floor(position));
    vec2 fraction = fract(position);
    vec3 color = vec3(0.0);
    float weightSum = 0.0;

    for (int y = 0; y < 2; ++y)
    {
        for (int x = 0; x < 2; ++x)
        {
            ivec2 pixel = clamp(base + ivec2(x, y), ivec2(0), size - ivec2(1));
            vec2 centerUV = (vec2(pixel) + 0.5) / vec2(size);
            if ((fetchDepth(centerUV) >= 1.0) != isSky) continue;

            float weight = (x == 0 ? 1.0 - fraction.x : fraction.x)
                         * (y == 0 ? 1.0 - fraction.y : fraction.y);
            color += texelFetch(colorTex, pixel, 0).rgb * weight;
            weightSum += weight;
        }
    }

    return weightSum > 0.0 ? color / weightSum : fallbackColor;
}

void main()
{
	vec2 texelSize = 1.0 / textureSize(colorTex, 0);

	float depth = fetchDepth(inUV);
	bool isSky = depth >= 1.0;
	ivec2 colorSize = textureSize(colorTex, 0);
	ivec2 colorPixel = clamp(ivec2(inUV * vec2(colorSize)), ivec2(0), colorSize - ivec2(1));
	vec3 centerColor = texelFetch(colorTex, colorPixel, 0).rgb;
	vec3 sharpColor = sampleSurfaceColor(inUV, isSky, centerColor);

	// A cubemap has no meaningful scene distance: neither fog nor DOF should
	// treat its far-plane depth as the distance to background scenery.
	if (isSky)
	{
		if (pp.enableTonemap != 0)
			sharpColor = tonemapACES(sharpColor) * pp.exposure;
		outColor = vec4(sharpColor, 1.0);
		return;
	}

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

			// Do not bleed bright geometry into the unfogged background (or vice versa).
			bool sampleIsSky = fetchDepth(uv) >= 1.0;
			if (sampleIsSky != isSky) continue;

			blurColor += sampleSurfaceColor(uv, isSky, sharpColor);
			total += 1.0;
		}
	}

	blurColor = total > 0.0 ? blurColor / total : sharpColor;

	vec3 finalColor = mix(sharpColor, blurColor, blurAmount);

	if (pp.enableFog != 0 && !isSky)
	{
		float fogFactor = computeFogFactor(linearDepth);
		finalColor = mix(finalColor, pp.fogColor, fogFactor);
	}

	if(pp.enableTonemap != 0)
		finalColor = tonemapACES(finalColor) * pp.exposure;

	outColor = vec4(finalColor, 1.0);
}