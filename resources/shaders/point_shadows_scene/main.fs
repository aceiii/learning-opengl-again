#version 330 core

out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

uniform sampler2D texture_diffuse1;
uniform samplerCube depthMap;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform vec3 viewPos;

uniform float farPlane;

uniform bool usePCF;
uniform bool enableShadows;
uniform bool showDepth;

float ShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir) {
    vec3 fragToLight = fragPos - lightPos;

    float shadow = 0.0;
    float currentDepth = length(fragToLight);
    float bias = 0.05;
    float samples = 4.0;
    float offset = 0.1;

    if (usePCF && !showDepth) {
        for (float x = -offset; x < offset; x+= offset / (samples * 0.5)) {
            for (float y = -offset; y < offset; y += offset / (samples * 0.5)) {
                for (float z = -offset; z < offset; z += offset / (samples * 0.5)) {
                    float closestDepth = texture(depthMap, fragToLight + vec3(x, y, z)).r;
                    closestDepth *= farPlane;
                    if (currentDepth - bias > closestDepth) {
                        shadow += 1.0;
                    }
                }
            }
        }

        shadow /= (samples * samples * samples);
    } else {
        float closestDepth = texture(depthMap, fragToLight).r;
        closestDepth *= farPlane;
        shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

        if (showDepth) {
            FragColor = vec4(vec3(closestDepth / farPlane), 1.0);
        }
    }

    return shadow;
}

vec3 BlinnPhong(vec3 normal, vec3 fragPos, vec3 lightPos, vec3 lightDir, vec3 lightColor) {
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;

    float distance = length(lightPos - fragPos);
    float attenuation = 1.0 / distance;

    diffuse *= attenuation;
    specular *= attenuation;

    return diffuse + specular;
}

void main() {
    vec3 color = texture(texture_diffuse1, fs_in.TexCoords).rgb;
    vec3 normal = normalize(fs_in.Normal);
    vec3 ambient = ambientColor;
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);

    vec3 lighting = vec3(0.0);
    lighting += BlinnPhong(normal, fs_in.FragPos, lightPos, lightDir, lightColor);

    float shadow = enableShadows ? ShadowCalculation(fs_in.FragPos, normal, lightDir) : 0.0;
    color *= ambient + (1.0 - shadow) * lighting;

    if (!showDepth) {
        FragColor= vec4(color, 1.0);
    }
}
