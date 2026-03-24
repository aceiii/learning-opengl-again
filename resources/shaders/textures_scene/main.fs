#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D texture1;
uniform sampler2D texture2;

uniform float outputMix;
uniform float textureBlend;
uniform float vertexBlend;
uniform bool mirrorFace;

void main() {
    vec3 vertexColor = mix(vec3(1.0f, 1.0f, 1.0f), ourColor, vertexBlend);
    vec4 happyFace = texture(texture2, mirrorFace ? vec2(-TexCoord.x, TexCoord.y) : TexCoord);
    vec4 textureColor = mix(texture(texture1, TexCoord), happyFace, textureBlend) * vec4(vertexColor, 1.0);
    FragColor = mix(vec4(ourColor, 1.0), textureColor, outputMix);
}
