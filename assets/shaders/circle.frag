#version 300 es

precision highp float;

in vec2 vs_texcoord;
in vec4 vs_fillColor;
in vec4 vs_outlineColor;
in float vs_innerRadius;

out vec4 fragColor;

void main(void)
{
    const float Radius = 0.5;
    float Feather = 2.0 * max(fwidth(vs_texcoord.x), fwidth(vs_texcoord.y));

    float d = length(vs_texcoord - vec2(.5));
    float alpha = smoothstep(Radius, Radius - Feather, d);
    vec4 color = mix(vs_fillColor, vs_outlineColor, smoothstep(vs_innerRadius - 0.5 * Feather, vs_innerRadius + 0.5 * Feather, d));

    fragColor = vec4(color.xyz, alpha * color.a);
}
