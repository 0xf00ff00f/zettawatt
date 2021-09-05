#version 420 core

in vec2 vs_texcoord;
in vec4 vs_color;
in float vs_size;

out vec4 fragColor;

void main(void)
{
    const float Radius = 0.5;
    const float Feather = 2.0 / vs_size;

    float d = abs(vs_texcoord.y - .5);
    float c = smoothstep(Radius, Radius - Feather, d);

    fragColor = vec4(vs_color.xyz, c * vs_color.a);
}
