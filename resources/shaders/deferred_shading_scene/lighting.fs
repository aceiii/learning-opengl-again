#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform int debug;

uniform sampler2D texture_position;
uniform sampler2D texture_normal;
uniform sampler2D texture_albedo_spec;

void main() {
    // const float gamma = 2.2;
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

    FragColor = vec4(position, 1.0);
}
