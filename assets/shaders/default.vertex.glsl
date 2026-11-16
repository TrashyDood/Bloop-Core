#version 330 core

// bind vertex attributes
layout (location = 0) in vec3 vertex_position;
layout (location = 1) in vec3 vertex_normal;
layout (location = 2) in vec2 vertex_uv;
layout (location = 3) in vec4 vertex_color;
layout (location = 4) in vec3 instance_offset;

// "static" variables for this shader
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

// vertex 'out' variables are linked to fragment 'in' variables
out vec4 fragment_color;
out vec3 fragment_normal;
out vec2 fragment_uv;

void main()
{
    fragment_uv = vertex_uv;
    fragment_color = vertex_color;

    vec4 worldPosition = modelMatrix * vec4(vertex_position, 1.0);
    worldPosition.xyz += instance_offset;
    gl_Position = projectionMatrix * viewMatrix * worldPosition;

    mat3 modelMatrix3x3 = mat3(modelMatrix); // keep rotation and scale discard translation
    mat3 inverseModelMatrix = inverse(modelMatrix3x3); // reverse the model's rotation and scale
    mat3 normalMatrix = transpose(inverseModelMatrix); // adjust the transform for surface normals
    vec3 transformedNormal = normalMatrix * vertex_normal; // transform the vertex normal
    fragment_normal = normalize(transformedNormal); // keep the normal unit length
}
