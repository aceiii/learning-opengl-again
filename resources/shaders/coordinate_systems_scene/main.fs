#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D texture1;
uniform sampler2D texture2;

void main() {
    vec4 happyFace = texture(texture2, TexCoord);
    vec4 textureColor = mix(texture(texture1, TexCoord), happyFace, 0.2);
    FragColor = textureColor;
}
