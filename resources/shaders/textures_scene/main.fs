#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D texture1;
uniform sampler2D texture2;

uniform float outputMix;
uniform float textureBlend;
uniform float vertexBlend;

void main() {
    vec3 vertexColor = mix(vec3(1.0f, 1.0f, 1.0f), ourColor, vertexBlend);
    vec4 textureColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), textureBlend) * vec4(vertexColor, 1.0);
    FragColor = mix(vec4(ourColor, 1.0), textureColor, outputMix);
}
