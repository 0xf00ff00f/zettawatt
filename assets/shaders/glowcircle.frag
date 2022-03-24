#version 300 es

precision highp float;

in vec2 vs_texcoord;
in vec4 vs_glowColor;
in vec4 vs_bgColor;
in float vs_size;
in float vs_radius;
in float vs_glowDistance;
in float vs_glowStrength;

out vec4 fragColor;

void main(void)
{
    float radius = vs_radius / vs_size; /// in uv coords
    float d = length(vs_texcoord - vec2(0.5));

    // glow
    float x = abs(d - radius);
    float glow = vs_glowDistance / pow(x, vs_glowStrength);

    // alpha
    float r = min(d, 0.5) / 0.5;
    float alpha = 0.5 + 0.5 * cos(r * 3.1415);

    fragColor = vec4(vs_bgColor.xyz + vs_glowColor.xyz * glow, alpha);
}
