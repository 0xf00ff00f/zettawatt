#version 300 es

precision highp float;

in vec2 vs_texcoord;
in vec4 vs_startColor;
in vec4 vs_endColor;
in float vs_size;
in float vs_startAngle;
in float vs_endAngle;
in float vs_currentAngle;

out vec4 fragColor;

#define PI 3.14159265

void main(void)
{
    const float Radius = 0.5;
    const float Thickness = 8.0; // /in pixels
    float InnerRadius = 0.5 - Thickness / vs_size; // in uv coords
    float Feather = 2.0 / vs_size;
    vec4 InactiveColor = vec4(0.25, 0.25, 0.25, vs_endColor.w);

    vec2 p = vs_texcoord - vec2(.5);
    float angle = atan(p.y, p.x) + PI;

    if (step(vs_startAngle, angle) * step(angle, vs_endAngle) == 0.0)
        discard;

    float d = length(p);
    float alpha = smoothstep(Radius, Radius - Feather, d) * smoothstep(InnerRadius, InnerRadius + Feather, d);

    vec4 activeColor = mix(vs_startColor, vs_endColor, clamp((angle - vs_startAngle) / (vs_currentAngle - vs_startAngle), 0.0, 1.0));
    float AngleFeather = 0.05;
    vec4 color = mix(activeColor, InactiveColor, smoothstep(vs_currentAngle - AngleFeather, vs_currentAngle, angle));

    fragColor = vec4(color.xyz, alpha * color.w);
}
