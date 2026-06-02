#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform float exposure;
uniform bool enableBloom;
uniform bool enableScene;

uniform sampler2D texture_scene;
uniform sampler2D texture_blur;

void main() {
    const float gamma = 2.2;
    vec3 hdrColor = texture(texture_scene, TexCoords).rgb;
    vec3 bloomColor = texture(texture_blur, TexCoords).rgb;

    if (!enableScene) {
        hdrColor = vec3(0.0);
    }

    if (enableBloom) {
        hdrColor += bloomColor;
    }

    vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
    result = pow(result, vec3(1.0 / gamma));
    FragColor = vec4(result, 1.0);
}
