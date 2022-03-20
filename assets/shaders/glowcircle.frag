#version 300 es

precision highp float;

in vec2 vs_texcoord;
in vec4 vs_color;
in float vs_size;
in float vs_radius;
in float vs_glowDistance;
in float vs_glowStrength;

out vec4 fragColor;

void main(void)
{
    float radius = vs_radius / vs_size; /// in uv coords
    float x = abs(length(vs_texcoord - vec2(0.5)) - radius);
    float glow = vs_glowDistance / pow(x, vs_glowStrength);
    fragColor = vs_color * glow;
}
