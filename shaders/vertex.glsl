#version 330 core
layout (location = 0) in vec2 vertex_position;
uniform vec2 position;
uniform vec2 scale;
uniform float rotation;

void main()
{
    vec2 point = vertex_position * scale;
    float c = cos(rotation);
    float s = sin(rotation);
    point = mat2(c, s, -s, c) * point;
    gl_Position = vec4(point + position, 0.0, 1.0);
}
