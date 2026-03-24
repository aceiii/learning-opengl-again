#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D ourTexture;
uniform float textureBlend;
uniform float vertexBlend;

void main() {
    vec3 vertexColor = mix(vec3(1.0f, 1.0f, 1.0f), ourColor, vertexBlend);
    vec4 textureColor = texture(ourTexture, TexCoord) * vec4(vertexColor, 1.0);
    FragColor = mix(vec4(ourColor, 1.0), textureColor, textureBlend);
}
