#version 330 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedo;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

uniform sampler2D texture_diffuse;
uniform sampler2D texture_specular;

void main() {
    gPosition = fs_in.FragPos;
    gNormal = normalize(fs_in.Normal);
    gAlbedo.rgb = vec3(0.95);
}
