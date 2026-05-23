#version 330 core

out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} fs_in;

uniform sampler2D texture_diffuse;
uniform sampler2D texture_normal;
uniform sampler2D texture_depth;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 ambientColor;

uniform float heightScale;

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir) {
    float height = texture(texture_depth, texCoords).r;
    vec2 p = viewDir.xy / viewDir.z * (height * heightScale);
    return texCoords - p;
}

vec3 BlinnPhong(vec3 normal, vec3 fragPos, vec3 viewDir, vec3 lightPos, vec3 lightDir, vec3 lightColor) {
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = spec * vec3(0.2);

    float distance = length(lightPos - fragPos);
    float attenuation = 1.0 / distance;

    diffuse *= attenuation;
    specular *= attenuation;

    return diffuse + specular;
}

void main() {
    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec2 texCoords = ParallaxMapping(fs_in.TexCoords, viewDir);
    if (texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0) {
        discard;
    }

    vec3 color = texture(texture_diffuse, texCoords).rgb;
    vec3 normal = normalize(texture(texture_normal, texCoords).rgb * 2.0 - 1.0);
    vec3 ambient = ambientColor;
    vec3 lightDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);

    vec3 lighting = BlinnPhong(normal, fs_in.FragPos, viewDir, lightPos, lightDir, lightColor);
    color *= ambient + lighting;

    FragColor = vec4(color, 1.0);
}
