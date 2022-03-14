#version 420 core

layout(location=0) in vec2 position;
layout(location=1) in vec2 texcoord;
layout(location=2) in vec4 fgColor;
layout(location=3) in vec4 bgColor;
layout(location=4) in vec4 size;

uniform mat4 modelViewProjection;

out vec2 vs_texcoord;
out vec4 vs_fillColor;
out vec4 vs_outlineColor;
out float vs_size;
out float vs_outlineSize;

void main(void)
{
    vs_texcoord = texcoord;
    vs_fillColor = fgColor;
    vs_outlineColor = bgColor;
    vs_size = size.x;
    vs_outlineSize = size.y;
    gl_Position = modelViewProjection * vec4(position, 0, 1);
}
