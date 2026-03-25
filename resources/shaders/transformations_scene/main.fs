#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D texture1;
uniform sampler2D texture2;
uniform float textureBlend;

void main() {
    vec4 happyFace = texture(texture2, TexCoord);
    vec4 textureColor = mix(texture(texture1, TexCoord), happyFace, textureBlend);
    FragColor = textureColor;
}
