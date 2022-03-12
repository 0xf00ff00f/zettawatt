#version 420 core

in vec2 vs_texcoord;
in vec4 vs_color;
in float vs_size;
in float vs_radius;

out vec4 fragColor;

void main(void)
{
    const float radius = vs_radius / vs_size; /// in uv coords
    float x = abs(length(vs_texcoord - vec2(0.5)) - radius);
    float glow = 0.06 / pow(x, 0.6);
    fragColor = vs_color * glow;
}
