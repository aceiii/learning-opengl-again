#version 330 core

#define PI 3.1415926535897932384626433832795

out vec4 FragColor;

in vec2 TexCoords;

uniform int width;
uniform int height;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 viewPos;

float DistributeGGX(vec3 N, vec3 H, float a) {
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float nom = a2;
    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float k) {
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float k) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, k);
    float ggx2 = GeometrySchlickGGX(NdotL, k);
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
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

void main() {
    vec2 pos = vec2(TexCoords.x, -TexCoords.y + 1.0) * vec2(width, height);

    vec3 result = vec3(sphere(translate(pos, vec2(width / 2, height / 2)), 100.0), 0.0, 0.0);
    result *= vec3(box(rotate(translate(pos, vec2(200, 200)), 3.14159/12), vec2(250, 325)), 0.0, 0.0);

    if (result.x >= 1.0) {
        result.y = abs(result.x) / 100.0;
    }

    FragColor = vec4(sin(result / 1000.0), 1.0);
    // FragColor = vec4(result, 1.0);
}
