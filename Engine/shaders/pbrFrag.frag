#version 450

layout(push_constant) uniform PushConstants {
    mat4 transform;
    vec3 color;
} pushConstants;

layout(set = 1, binding = 0) uniform UBO {
    mat4 view;
    mat4 proj;
    vec3 camPos;
} ubo;

layout(set = 1, binding = 1) uniform FLAGS {
    int hasDiffuseMap;    // 4 bytes
    int hasNormalMap;     // 4 bytes
    int hasRoughnessMap;  // 4 bytes
    int hasMetallicMap;   // 4 bytes
    int hasAOMap;         // 4 bytes
    float metallic;       // 4 bytes
    float roughness;      // 4 bytes
    float ao;             // 4 bytes
} flags;


layout(set = 1, binding = 2) uniform sampler2D albedoMap;
layout(set = 1, binding = 3) uniform sampler2D normalMap;
layout(set = 1, binding = 4) uniform sampler2D metallicMap;
layout(set = 1, binding = 5) uniform sampler2D roughnessMap;
layout(set = 1, binding = 6) uniform sampler2D aoMap;

layout(set = 0, binding = 0) uniform samplerCube irradianceDiffuseMap;
layout(set = 0, binding = 1) uniform samplerCube irradianceSpecularMap;
layout(set = 0, binding = 2) uniform sampler2D BRDFMap;

layout(location = 0) in vec3 WorldPos;
layout(location = 1) in vec2 TexCoords;
layout(location = 2) in vec3 Normal;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

vec3 lightColors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

vec3 lightPositions[3] = vec3[](
    vec3(0.5, 3.0, 0.0),
    vec3(0.5, 2.5, 0.5),
    vec3(0.0, 4.0, 0.0)
);

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}


void main()
{	
    vec3 albedo = pushConstants.color;
    if(flags.hasDiffuseMap != 0)
        albedo = texture(albedoMap, TexCoords).rgb;
    
    vec3 N = Normal;
    if(flags.hasNormalMap != 0)
        N = getNormalFromMap();

    float metallic = flags.metallic;
    if(flags.hasMetallicMap != 0)
        metallic  = texture(metallicMap, TexCoords).r;
    
    float roughness = flags.roughness;
    if(flags.hasRoughnessMap != 0)
        roughness = texture(roughnessMap, TexCoords).r;
    
    float ao = flags.ao;
    if(flags.hasAOMap != 0)
        ao = texture(aoMap, TexCoords).r;

    vec3 V = normalize(ubo.camPos - WorldPos);
    vec3 R = reflect(-V, N);
    R.x = -R.x;

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 

    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    
    for(int i = 0; i < 3; ++i) 
    {
        // calculate per-light radiance
        vec3 L = normalize(lightPositions[i] - WorldPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPositions[i] - WorldPos);
        //float attenuation = 1.0 / (distance * distance);
        float attenuation = 1.0 / log(distance + 1.0);
        vec3 radiance = lightColors[i] * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;
        
        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;	  

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);        

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }   
    
    // ambient lighting (note that the next IBL tutorial will replace 
    // this ambient lighting with environment lighting).
    // vec3 ambient = vec3(0.03) * albedo * ao;

    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    vec3 irradiance = texture(irradianceDiffuseMap, N).rgb;
    vec3 diffuse    = irradiance * albedo;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(irradianceSpecularMap, R,  roughness * MAX_REFLECTION_LOD).rgb;   
    vec2 envBRDF  = texture(BRDFMap, vec2(max(dot(N, V), 0.0), roughness)).rg;
    float specularFade = 1.0 - roughness * roughness;
    vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y) * specularFade;

    vec3 ambient = (kD * diffuse + specular) * ao; 
    
    vec3 color = ambient + Lo;
    
    // HDR tone mapping
    color = color / (color + vec3(1.0));

    // Gamma correction (assuming gamma = 2.2)
    //color = pow(color, vec3(1.0 / 2.2)); 

    outColor = vec4(color, 1.0);

}
