#version 330 core

const float PI = 3.14159265359;

out vec4 FragColor;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

uniform sampler2D texture_albedo;
uniform sampler2D texture_normal;
uniform sampler2D texture_metallic;
uniform sampler2D texture_roughness;
uniform sampler2D texture_ao;

uniform vec3 viewPos;

struct Light {
    vec3 Position;
    vec3 Color;
};

const int NUM_LIGHTS = 4;

uniform Light lights[NUM_LIGHTS];
uniform vec3 ambient;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float num = a2;
    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float sphere(vec2 p, float r) {
    return length(p) - r;
}

float box(vec2 p, vec2 size) {
    vec2 d = abs(p) - (size / 2.0);
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

vec2 translate(vec2 a, vec2 b) {
    return a - b;
}

vec2 rotate(vec2 p, float r) {
    float s = sin(r);
    float c = cos(r);
    return vec2(c * p.x + s * p.y, c * p.y - s * p.x);
}

vec3 GetNormalFromMap() {
    vec3 tangentNormal = texture(texture_normal, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1 = dFdx(WorldPos);
    vec3 Q2 = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N = normalize(Normal);
    vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main() {
    vec3 albedo = pow(texture(texture_albedo, TexCoords).rgb, vec3(2.2));
    float metallic = texture(texture_metallic, TexCoords).r;
    float roughness = texture(texture_roughness, TexCoords).r;
    float ao = texture(texture_ao, TexCoords).r;

    vec3 N = GetNormalFromMap();
    vec3 V = normalize(viewPos - WorldPos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);
    for (int i = 0; i < NUM_LIGHTS; i++) {
        vec3 L = normalize(lights[i].Position - WorldPos);
        vec3 H = normalize(V + L);

        float distance = length(lights[i].Position - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lights[i].Color * attenuation;

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = FresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 color = (ambient * albedo * ao) + Lo;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(Lo, 1.0);
}
