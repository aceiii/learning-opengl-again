#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform int debug;

uniform sampler2D texture_ssao;

uniform vec3 viewPos;

void main() {
    float ssao = texture(texture_ssao, TexCoords).r;
    FragColor = vec4(vec3(ssao), 1.0);
}
