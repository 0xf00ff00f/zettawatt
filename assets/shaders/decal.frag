#version 420 core

uniform sampler2D baseColorTexture;

in vec2 vs_texcoord;

out vec4 fragColor;

void main(void)
{
    fragColor = texture(baseColorTexture, vs_texcoord);
}
