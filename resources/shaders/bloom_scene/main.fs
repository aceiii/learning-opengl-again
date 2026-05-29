#version 330 core

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

struct Light {
    vec3 Position;
    vec3 Color;
};

uniform Light lights[16];
uniform vec3 ambientColor;
uniform vec3 viewPos;

uniform sampler2D texture_diffuse;

vec3 BlinnPhong(vec3 color, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 lightPos, vec3 lightDir, vec3 lightColor) {
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor * color;
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = spec * vec3(0.2);

    float distance = length(fragPos - lightPos);
    float attenuation = 1.0 / (distance * distance);

    diffuse *= attenuation;
    specular *= attenuation;

    return diffuse + specular;
}

void main() {
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 color = texture(texture_diffuse, fs_in.TexCoords).rgb;
    vec3 ambient = ambientColor;
    vec3 lighting = vec3(0.0);

    for (int i = 0; i < 16; i++) {
        Light light = lights[i];
        vec3 lightDir = normalize(light.Position - fs_in.FragPos);
        lighting += BlinnPhong(color, fs_in.Normal, fs_in.FragPos, viewDir, light.Position, lightDir, light.Color);
    }

    vec3 result = ambient + lighting;
    FragColor = vec4(result, 1.0);

    float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0) {
        BrightColor = vec4(result, 1.0);
    } else {
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
