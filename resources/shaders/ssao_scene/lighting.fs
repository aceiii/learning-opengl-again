#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform int debug;

uniform sampler2D texture_position;
uniform sampler2D texture_normal;
uniform sampler2D texture_albedo_spec;

struct Light {
    vec3 Position;
    vec3 Color;
    float Radius;
};

const int NUM_LIGHTS = 32;

uniform Light lights[NUM_LIGHTS];
uniform vec3 viewPos;

vec3 BlinnPhong(vec3 color, float spec, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 lightPos, vec3 lightDir, vec3 lightColor) {
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor * color;
    vec3 specular = spec * vec3(0.2);

    float distance = length(fragPos - lightPos);
    float attenuation = 1.0 / (distance * distance);

    diffuse *= attenuation;
    specular *= attenuation;

    return diffuse + specular;
}

void main() {
    vec3 position = texture(texture_position, TexCoords).rgb;
    vec3 normal = texture(texture_normal, TexCoords).rgb;
    vec3 albedo = texture(texture_albedo_spec, TexCoords).rgb;
    float spec = texture(texture_albedo_spec, TexCoords).a;

    if (debug > 0) {
        switch (debug) {
            case 1: FragColor = vec4(position, 1.0); break;
            case 2: FragColor = vec4(normal, 1.0); break;
            case 3: FragColor = vec4(albedo, 1.0); break;
            case 4: FragColor = vec4(spec, spec, spec, 1.0); break;
        }
        return;
    }

    vec3 viewDir = normalize(viewPos - position);
    vec3 ambient = albedo * 0.1;
    vec3 lighting = vec3(0.0);

    for (int i = 0; i < NUM_LIGHTS; i++) {
        Light light = lights[i];

        float distance = length(light.Position - position);
        if (distance >= light.Radius) {
            continue;
        }

        vec3 lightDir = normalize(light.Position - position);
        lighting += BlinnPhong(albedo, spec, normal, position, viewDir, light.Position, lightDir, light.Color);
    }

    vec3 result = ambient + lighting;
    FragColor = vec4(result, 1.0);
}
