#version 420 core

in vec2 vs_texcoord;
in vec4 vs_fillColor;
in vec4 vs_outlineColor;
in float vs_size;
in float vs_outlineSize;

out vec4 fragColor;

void main(void)
{
    const float Radius = 0.5;
    const float InnerRadius = 0.5 - vs_outlineSize / vs_size; // in uv coords
    const float Feather = 2.0 / vs_size;

    float d = length(vs_texcoord - vec2(.5));
    float alpha = smoothstep(Radius, Radius - Feather, d);
    vec4 color = mix(vs_fillColor, vs_outlineColor, smoothstep(InnerRadius, InnerRadius + Feather, d));

    fragColor = vec4(color.xyz, alpha * color.a);
}
