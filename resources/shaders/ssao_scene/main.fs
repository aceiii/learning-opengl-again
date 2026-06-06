#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

struct Light {
    vec3 Position;
    vec3 Color;
};

uniform Light lights[16];
uniform vec3 ambientColor;
uniform vec3 viewPos;

uniform sampler2D texture_position;
uniform sampler2D texture_normal;
uniform sampler2D texture_albedo_spec;
uniform sampler2D texture_ssao;

vec3 BlinnPhong(vec3 color, float spec, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 lightPos, vec3 lightDir, vec3 lightColor) {
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor * color;
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    // float spec1 = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = spec * vec3(0.2);

    float distance = length(fragPos - lightPos);
    float attenuation = 1.0 / (distance * distance);

    diffuse *= attenuation;
    specular *= attenuation;

    return diffuse;// + specular;
}

void main() {
    vec3 fragPos = texture(texture_position, TexCoords).rgb;
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 color = texture(texture_albedo_spec, TexCoords).rgb;
    float spec = texture(texture_albedo_spec, TexCoords).a;
    vec3 normal = texture(texture_normal, TexCoords).rgb;
    float occlusion = texture(texture_ssao, TexCoords).r;
    vec3 ambient = ambientColor * occlusion * color;
    vec3 lighting = vec3(0.0);

    for (int i = 0; i < 16; i++) {
        Light light = lights[i];
        vec3 lightDir = normalize(light.Position - fragPos);
        lighting += BlinnPhong(color, spec, normal, fragPos, viewDir, light.Position, lightDir, light.Color);
    }

    // float occlusion = texture(texture_ssao, TexCoords).r;
    // vec3 result = texture(texture_albedo_spec, TexCoords).rgb * occlusion;
    vec3 result = ambient + lighting;
    FragColor = vec4(result, 1.0);
}
