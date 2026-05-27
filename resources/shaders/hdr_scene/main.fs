#version 330 core

out vec4 FragColor;

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

    // return diffuse + specular;
    return diffuse;
}

void main2() {
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 color = texture(texture_diffuse, fs_in.TexCoords).rgb;
    vec3 ambient = ambientColor;
    vec3 lighting = vec3(0.0);

    for (int i = 0; i < 16; i++) {
        Light light = lights[i];
        vec3 lightDir = normalize(light.Position - fs_in.FragPos);
        lighting += BlinnPhong(color, fs_in.Normal, fs_in.FragPos, viewDir, light.Position, lightDir, light.Color);
    }
    FragColor = vec4(ambient + lighting, 1.0);
}

void main()
{
    vec3 color = texture(texture_diffuse, fs_in.TexCoords).rgb;
    vec3 normal = normalize(fs_in.Normal);
    // ambient
    vec3 ambient = 0.0 * color;
    // lighting
    vec3 lighting = vec3(0.0);
    for(int i = 0; i < 16; i++)
    {
        // diffuse
        vec3 lightDir = normalize(lights[i].Position - fs_in.FragPos);
        float diff = max(dot(lightDir, normal), 0.0);
        vec3 diffuse = lights[i].Color * diff * color;
        vec3 result = diffuse;
        // attenuation (use quadratic as we have gamma correction)
        float distance = length(fs_in.FragPos - lights[i].Position);
        result *= 1.0 / distance; //(distance * distance);
        lighting += result;

    }
    FragColor = vec4(ambient + lighting, 1.0);
}
