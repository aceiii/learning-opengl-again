#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture1;
uniform bool viewDepth;
uniform float depthScale;

float near = 0.1;
float far = 100.0;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main() {
    if (viewDepth) {
        float depth = LinearizeDepth(gl_FragCoord.z) / far;
        FragColor = vec4(vec3(depth * depthScale), 1.0);
    } else {
        FragColor = texture(texture1, TexCoords);
    }

}
