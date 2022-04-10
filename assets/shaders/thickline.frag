#version 300 es

precision highp float;

in vec2 vs_texcoord;
in vec4 vs_fromColor;
in vec4 vs_toColor;

out vec4 fragColor;

void main(void)
{
    const float Radius = 0.5;
    float Feather = fwidth(vs_texcoord.y);

    float d = abs(vs_texcoord.y - .5);
    float c = smoothstep(Radius, Radius - Feather, d);
    vec4 color = mix(vs_fromColor, vs_toColor, vs_texcoord.x);

    fragColor = vec4(color.xyz, c * color.a);
}
