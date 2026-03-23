#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 ourColor;

uniform float yScale;
uniform float xOffset;
uniform float colorMix;

void main() {
    gl_Position = vec4(aPos.x + xOffset, aPos.y * yScale, aPos.z, 1.0);
    ourColor = mix(aColor, gl_Position.xyz, colorMix);
}
