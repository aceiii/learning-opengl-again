#version 330 core

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

in VS_OUT {
    vec3 normal;
} gs_in[];

uniform float time;
uniform mat4 projection;

const float MAGNITUDE = 0.4;


vec3 GetNormal() {
    vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
    vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
    return normalize(cross(a, b));
}

vec4 explode(vec4 position, vec3 normal) {
    float magnitude = 2.0;
    vec3 direction = normal * ((time + 1.0) / 2.0) * magnitude;
    return position + vec4(direction, 0.0);
}

void GenerateLine(int index) {
    vec3 normal = GetNormal();

    gl_Position = explode(gl_in[index].gl_Position, normal);
    EmitVertex();

    gl_Position =  explode(gl_in[index].gl_Position + vec4(gs_in[index].normal, 0.0) * MAGNITUDE, normal);
    EmitVertex();

    EndPrimitive();
}

void main() {
    GenerateLine(0);
    GenerateLine(1);
    GenerateLine(2);
}
