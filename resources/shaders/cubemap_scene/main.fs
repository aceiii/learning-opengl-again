#version 330 core

out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 Position;

uniform vec3 viewPos;
uniform samplerCube skybox;
uniform bool useReflect;
uniform float refractiveIndex;

void main() {
    if (useReflect) {
        vec3 I = normalize(Position - viewPos);
        vec3 R = reflect(I, normalize(Normal));
        FragColor = vec4(texture(skybox, R).rgb, 1.0);
    } else {
        float ratio = 1.0 / refractiveIndex;
        vec3 I = normalize(Position - viewPos);
        vec3 R = refract(I, normalize(Normal), ratio);
        FragColor = vec4(texture(skybox, R).rgb, 1.0);
    }
}
