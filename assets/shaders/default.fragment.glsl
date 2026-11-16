#version 330 core

// fragment 'in' variables are linked to vertex 'out' variables
in vec4 fragment_color;
in vec3 fragment_normal;
in vec2 fragment_uv;

out vec4 output_color;

uniform vec3 light_direction = vec3(-0.4, 0.8, 0.6);
uniform sampler2D diffuseTexture;

void main()
{
    float diffuse = max(dot(normalize(fragment_normal), normalize(light_direction)), 0.0);
    vec3 lighting = vec3(0.2) + vec3(0.8) * diffuse;
    vec4 texel = texture(diffuseTexture, fragment_uv);

    output_color = vec4(texel.rgb * fragment_color.rgb * lighting, texel.a * fragment_color.a);
}
