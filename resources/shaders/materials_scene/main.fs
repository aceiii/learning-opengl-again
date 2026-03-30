#version 330 core
struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 viewPos;
uniform Material material;
uniform Light light;

void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    vec3 viewDir = normalize(viewPos - FragPos);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = material.specular  * spec * light.specular;

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = material.diffuse * diff * light.diffuse;

    vec3 ambient = material.ambient * light.ambient;

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
