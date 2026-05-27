#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D hdrBuffer;

void main() {
    const float gamma = 2.2;
    vec3 hdrColor = texture(hdrBuffer, TexCoords).rgb;
    FragColor = vec4(pow(hdrColor, vec3(1.0 / gamma)), 1.0);
}
