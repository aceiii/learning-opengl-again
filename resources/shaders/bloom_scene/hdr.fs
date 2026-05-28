#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform float exposure;
uniform bool enableHDR;

uniform sampler2D hdrBuffer;

void main() {
    const float gamma = 2.2;
    vec3 hdrColor = texture(hdrBuffer, TexCoords).rgb;
    if (enableHDR) {
        vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
        vec3 result = pow(mapped, vec3(1.0 / gamma));
        FragColor = vec4(result, 1.0);
    } else {
        vec3 result = pow(hdrColor, vec3(1.0 / gamma));
        FragColor = vec4(result, 1.0);
    }
}
