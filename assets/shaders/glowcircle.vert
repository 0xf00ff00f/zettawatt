#version 300 es

layout(location=0) in vec2 position;
layout(location=1) in vec2 texcoord;
layout(location=2) in vec4 fgColor;
layout(location=3) in vec4 bgColor;

uniform mat4 modelViewProjection;

out vec2 vs_texcoord;
out vec4 vs_color;
out float vs_size;
out float vs_radius;
out float vs_glowDistance;
out float vs_glowStrength;

void main(void)
{
    vs_texcoord = texcoord;
    vs_color = fgColor;
    vs_size = bgColor.x;
    vs_radius = bgColor.y;
    vs_glowDistance = bgColor.z;
    vs_glowStrength = bgColor.w;
    gl_Position = modelViewProjection * vec4(position, 0, 1);
}
