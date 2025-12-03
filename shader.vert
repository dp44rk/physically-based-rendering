#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    mat3 TBN;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform vec3 viewPos;
uniform bool useTangentSpace;

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    // Normal matrix for world-space normal
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vs_out.Normal = normalize(normalMatrix * aNormal);
    vs_out.TexCoords = aTexCoords;
    
    if (useTangentSpace) {
        vec3 T = normalize(normalMatrix * aTangent);
        vec3 N = normalize(normalMatrix * aNormal);
        T = normalize(T - dot(T, N) * N);
        vec3 B = cross(N, T);
        
        vs_out.TBN = transpose(mat3(T, B, N));
        vs_out.TangentViewPos  = vs_out.TBN * viewPos;
        vs_out.TangentFragPos  = vs_out.TBN * vs_out.FragPos;
    } else {
        vs_out.TBN = mat3(1.0);
        vs_out.TangentViewPos = vec3(0.0);
        vs_out.TangentFragPos = vec3(0.0);
    }
    
    gl_Position = projection * view * vec4(vs_out.FragPos, 1.0);
}
