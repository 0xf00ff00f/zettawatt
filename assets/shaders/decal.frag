#version 300 es

precision highp float;

uniform sampler2D baseColorTexture;

in vec2 vs_texcoord;

out vec4 fragColor;

void main(void)
{
    fragColor = texture(baseColorTexture, vs_texcoord);
}
