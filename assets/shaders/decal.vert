#version 420 core

layout(location=0) in vec2 position;
layout(location=1) in vec2 texcoord;

uniform mat4 modelViewProjection;

out vec2 vs_texcoord;
out vec4 vs_color;

void main(void)
{
    vs_texcoord = texcoord;
    gl_Position = modelViewProjection * vec4(position, 0, 1);
}
