#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform int width;
uniform int height;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 viewPos;

float sphere(vec2 p, vec2 c, float r) {
    return clamp(length(p - c) - r, 0.0, 1.0);
}

float box(vec2 p, vec2 center, vec2 size) {
    vec2 d = abs(p - center) - (size / 2.0);
    return clamp(length(max(d, 0.0)) + min(max(d.x, d.y), 0.0), 0.0, 1.0);
}

void main() {
    vec2 pos = vec2(TexCoords.x, -TexCoords.y + 1.0) * vec2(width, height);

    vec3 result = vec3(sphere(pos, vec2(width / 2, height / 2), 100.0), 0.0, 0.0);
    result *= vec3(box(pos, vec2(200, 200), vec2(250, 325)), 0.0, 0.0);

    FragColor = vec4(result, 1.0);
}
