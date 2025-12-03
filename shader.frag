#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 Normal;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    mat3 TBN;
} fs_in;

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

// camera position (world space)
uniform vec3 viewPos;

uniform bool hasAlbedoMap;
uniform bool hasNormalMap;
uniform bool hasMetallicMap;
uniform bool hasRoughnessMap;
uniform bool hasAoMap;
uniform bool useTangentSpace;
uniform bool useIBL;
uniform bool albedoIsSRGB;

// 기본 Material 값 (맵이 없을 때 사용)
uniform vec3 albedo;
uniform float metallic;
uniform float roughness;
uniform float ao;

uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];
uniform int numLights;

const float PI = 3.14159265359;

// Normal Distribution Function (GGX/Trowbridge-Reitz)
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return num / denom;
}

// Geometry Function (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

// Geometry Function (Smith)
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// Fresnel-Schlick 근사
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    // Material 속성 가져오기
    vec3 albedoColor = albedo;
    float metallicValue = metallic;
    float roughnessValue = roughness;
    float aoValue = ao;
    
    if (hasAlbedoMap) {
        albedoColor = texture(albedoMap, fs_in.TexCoords).rgb;
        if (albedoIsSRGB) {
            albedoColor = pow(albedoColor, vec3(2.2)); // sRGB -> linear
        }
    }
    if (hasMetallicMap) {
        metallicValue = texture(metallicMap, fs_in.TexCoords).r;
    }
    if (hasRoughnessMap) {
        roughnessValue = texture(roughnessMap, fs_in.TexCoords).r;
    }
    if (hasAoMap) {
        aoValue = texture(aoMap, fs_in.TexCoords).r;
    }
    
    // Normal mapping - tangent space에서 기본 normal은 (0,0,1)
    vec3 N;
    vec3 V;
    if (useTangentSpace) {
        N = vec3(0.0, 0.0, 1.0);
        if (hasNormalMap) {
            vec3 normalSample = texture(normalMap, fs_in.TexCoords).rgb;
            N = normalSample * 2.0 - 1.0; // [0,1] -> [-1,1]
            N = normalize(N);
        }
        V = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    } else {
        N = normalize(fs_in.Normal);
        V = normalize(viewPos - fs_in.FragPos);
    }
    
    // Dielectric F0 (0.04) 또는 Metallic F0 (albedo)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedoColor, metallicValue);
    vec3 FresnelV = fresnelSchlick(max(dot(N, V), 0.0), F0);
    vec3 kSBase = FresnelV;
    vec3 kDBase = (vec3(1.0) - kSBase) * (1.0 - metallicValue);
    
    // 조명 계산
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < numLights; ++i)
    {
        vec3 L;
        float distance;
        if (useTangentSpace) {
            vec3 worldLightPos = lightPositions[i];
            vec3 tangentLightPos = fs_in.TBN * worldLightPos;
            L = normalize(tangentLightPos - fs_in.TangentFragPos);
            distance = length(worldLightPos - fs_in.FragPos);
        } else {
            L = normalize(lightPositions[i] - fs_in.FragPos);
            distance = length(lightPositions[i] - fs_in.FragPos);
        }
        vec3 H = normalize(V + L);
        
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColors[i] * attenuation;
        
        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughnessValue);
        float G = GeometrySmith(N, V, L, roughnessValue);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallicValue;
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedoColor / PI + specular) * radiance * NdotL;
    }
    
    vec3 ambient;
    if (useIBL) {
        // IBL diffuse
        vec3 irradiance = texture(irradianceMap, N).rgb;
        vec3 diffuse = irradiance * albedoColor;
        
        // IBL specular
        vec3 R = reflect(-V, N);
        vec3 prefilteredColor = textureLod(prefilterMap, R, roughnessValue * 4.0).rgb;
        vec2 envBRDF = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughnessValue)).rg;
        vec3 specularIBL = prefilteredColor * (FresnelV * envBRDF.x + envBRDF.y);
        
        ambient = (kDBase * diffuse + specularIBL) * aoValue;
    } else {
        ambient = vec3(0.2) * albedoColor * aoValue;
    }
    
    vec3 color = ambient + Lo;
    
    // 최소 밝기 보장 (디버깅용)
    color = max(color, vec3(0.1) * albedoColor);
    
    // HDR tone mapping
    color = color / (color + vec3(1.0));
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, 1.0);
}
