#version 330 core

in vec3 vertex_position;
in vec3 vertex_normal;
in vec2 vertex_uv;
in vec4 vertex_color;
in vec3 instance_offset;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec4 fragment_color;
out vec3 fragment_normal;
out vec2 fragment_uv;

void main()
{
    fragment_uv = vertex_uv;
    fragment_color = vertex_color;

    vec3 transformedNormal = transpose(inverse(mat3(modelMatrix))) * vertex_normal;
    fragment_normal = normalize(transformedNormal);

    vec4 worldPosition = modelMatrix * vec4(vertex_position, 1.0);
    worldPosition.xyz += instance_offset;
    gl_Position = projectionMatrix * viewMatrix * worldPosition;
}
