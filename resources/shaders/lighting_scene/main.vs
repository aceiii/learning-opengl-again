#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;
out vec3 LightPos;

uniform vec3 lightPos;

uniform bool useViewSpace;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    if (useViewSpace) {
        gl_Position = projection * view * model * vec4(aPos, 1.0f);
        FragPos = vec3(view * model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(view * model))) * aNormal;
        LightPos = vec3(view * vec4(lightPos, 1.0));
    } else {
        gl_Position = projection * view * model * vec4(aPos, 1.0f);
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = aNormal;
        LightPos = lightPos;
    }
}
