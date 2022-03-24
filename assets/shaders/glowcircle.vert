#version 300 es

layout(location=0) in vec2 position;
layout(location=1) in vec2 texcoord;
layout(location=2) in vec4 fgColor;
layout(location=3) in vec4 bgColor;
layout(location=4) in vec4 size;

uniform mat4 modelViewProjection;

out vec2 vs_texcoord;
out vec4 vs_glowColor;
out vec4 vs_bgColor;
out float vs_size;
out float vs_radius;
out float vs_glowDistance;
out float vs_glowStrength;

void main(void)
{
    vs_texcoord = texcoord;
    vs_glowColor = fgColor;
    vs_bgColor = bgColor;
    vs_size = size.x;
    vs_radius = size.y;
    vs_glowDistance = size.z;
    vs_glowStrength = size.w;
    gl_Position = modelViewProjection * vec4(position, 0, 1);
}
