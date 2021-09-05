#version 420 core

in vec2 vs_texcoord;
in vec4 vs_color;
in float vs_size;

out vec4 fragColor;

void main(void)
{
    const float Radius = 0.5;

    const float Border = 6; // pixels
    const float InnerRadius = 0.5 - Border / vs_size; // in uv coords

    const float Feather = 2.0 / vs_size;

    float d = length(vs_texcoord - vec2(.5));
    float c = smoothstep(Radius, Radius - Feather, d) *
        smoothstep(InnerRadius, InnerRadius + Feather, d);

    fragColor = vec4(vs_color.xyz, c * vs_color.a);
}
