#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;
out vec3 LightPos;
out vec3 VertexColor;

uniform bool useViewSpace;
uniform bool useGouraudShading;
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float ambientStrength;
uniform float diffuseStrength;
uniform float specularStrength;

void main() {
    if (useGouraudShading) {
        vec3 fragPos = vec3(model * vec4(aPos, 1.0));

        vec3 norm = normalize(aNormal);
        vec3 lightDir = normalize(lightPos - fragPos);
        vec3 viewDir = normalize(viewPos - fragPos);
        vec3 reflectDir = reflect(-lightDir, norm);

        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = specularStrength  * spec * lightColor;

        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diffuseStrength * diff * lightColor;

        vec3 ambient = ambientStrength * lightColor;

        VertexColor = (ambient + diffuse + specular) * objectColor;

        gl_Position = projection * view * model * vec4(aPos, 1.0f);
    } else {
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
}
