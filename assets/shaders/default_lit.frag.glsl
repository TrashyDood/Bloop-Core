#version 330 core

in vec4 fragment_color;
in vec3 fragment_normal;
in vec2 fragment_uv;

out vec4 output_color;

const vec3 light_direction = vec3(-0.4, 0.8, 0.6);
const vec3 light_color = vec3(1.0, 1.0, 1.0);
const vec3 ambient_color = vec3(0.025, 0.025, 0.025);
uniform sampler2D textureSampler;

void main()
{
    float diffuse = max(dot(normalize(fragment_normal), normalize(light_direction)), 0.0);
    vec3 lighting = ambient_color + light_color * diffuse;
    vec4 texel = texture(textureSampler, fragment_uv);

    output_color = vec4(texel.rgb * fragment_color.rgb * lighting, texel.a * fragment_color.a);
}
