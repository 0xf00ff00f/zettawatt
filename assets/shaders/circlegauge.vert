#version 300 es

layout(location=0) in vec2 position;
layout(location=1) in vec2 texcoord;
layout(location=2) in vec4 fgColor;
layout(location=3) in vec4 bgColor;
layout(location=4) in vec4 size;

uniform mat4 modelViewProjection;

out vec2 vs_texcoord;
out vec4 vs_startColor;
out vec4 vs_endColor;
out float vs_size;
out float vs_startAngle;
out float vs_endAngle;
out float vs_currentAngle;

void main(void)
{
    vs_texcoord = texcoord;
    vs_startColor = fgColor;
    vs_endColor = bgColor;
    vs_size = size.x;
    vs_startAngle = size.y;
    vs_endAngle = size.z;
    vs_currentAngle = size.w;
    gl_Position = modelViewProjection * vec4(position, 0, 1);
}
