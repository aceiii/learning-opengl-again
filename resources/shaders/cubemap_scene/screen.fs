#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform int mode;
uniform float[9] kernel;

void RenderRegularScene() {
    FragColor = texture(screenTexture, TexCoords);
}

void RenderInverted() {
    FragColor = vec4(vec3(1.0 - texture(screenTexture, TexCoords)), 1.0);
}

void RenderGreyscale1() {
    FragColor = texture(screenTexture, TexCoords);
    float average = (FragColor.r + FragColor.g + FragColor.b) / 3.0;
    FragColor = vec4(average, average, average, 1.0);
}

void RenderGreyscale2() {
    FragColor = texture(screenTexture, TexCoords);
    float average = 0.2126 * FragColor.r + 0.7512 * FragColor.g + 0.0722 * FragColor.b;
    FragColor = vec4(average, average, average, 1.0);
}

const float offset = 1.0 / 300.0;

void RenderKernel() {
    vec2 offsets[9] = vec2[](
        vec2(-offset,  offset),
        vec2( 0.0,     offset),
        vec2( offset,  offset),
        vec2(-offset,  0.0),
        vec2( 0.0,     0.0),
        vec2( offset,  0.0),
        vec2(-offset, -offset),
        vec2( 0.0,    -offset),
        vec2( offset, -offset)
    );

    vec3 sampleTex[9];
    for (int i = 0; i < 9; i++) {
        sampleTex[i] = vec3(texture(screenTexture, TexCoords.st + offsets[i]));
    }

    float kernel_total = 0.0f;
    for (int i = 0; i < 9; i++) {
        kernel_total += kernel[i];
    }

    kernel_total = max(abs(kernel_total), 1.0);

    vec3 col = vec3(0.0);
    for (int i = 0; i < 9; i++) {
        col += sampleTex[i] * kernel[i] / kernel_total;
    }

    FragColor = vec4(col, 1.0);
}

void main() {
    switch (mode) {
        case 0: RenderRegularScene(); break;
        case 1: RenderInverted(); break;
        case 2: RenderGreyscale1(); break;
        case 3: RenderGreyscale2(); break;
        case 4: RenderKernel(); break;
    }
}
