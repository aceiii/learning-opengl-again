#version 330 core

out float FragColor;

in vec2 TexCoords;

uniform sampler2D texture_position;
uniform sampler2D texture_normal;
uniform sampler2D texture_noise;

const int NUM_SAMPLES = 64;

uniform int kernelSize = 64;
uniform float radius = 0.5;
uniform float bias = 0.025;

uniform vec3 samples[NUM_SAMPLES];
uniform mat4 projection;

const vec2 noiseScale = vec2(800.0 / 4.0, 600.0 / 4.0);

void main() {
    vec3 fragPos = texture(texture_position, TexCoords).xyz;
    vec3 normal = normalize(texture(texture_normal, TexCoords).rgb);
    vec3 randomVec = normalize(texture(texture_noise, TexCoords * noiseScale).xyz);

    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for (int i = 0; i < kernelSize; i++) {
        vec3 samplePos = TBN * samples[i];
        samplePos = fragPos + samplePos * radius;

        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        float sampleDepth = texture(texture_position, offset.xy).z;
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));

        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / kernelSize);
    FragColor = occlusion;
}
