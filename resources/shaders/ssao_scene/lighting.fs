#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform int debug;

uniform sampler2D texture_ssao;

uniform vec3 viewPos;

void main() {
    float ssao = texture(texture_ssao, TexCoords).r;
    FragColor = vec4(vec3(ssao), 1.0);

    // vec2 texelSize = 1.0 / vec2(textureSize(texture_ssao, 0));
    // float result = 0.0;
    // for (int x = -2; x < 2; ++x)
    // {
    //     for (int y = -2; y < 2; ++y)
    //     {
    //         vec2 offset = vec2(float(x), float(y)) * texelSize;
    //         result += texture(texture_ssao, TexCoords + offset).r;
    //     }
    // }
    // result = result / (4.0 * 4.0);
    // FragColor = vec4(vec3(result), 1.0);
}
