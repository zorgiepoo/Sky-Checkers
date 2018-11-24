#version 330

in lowp vec2 texVarying;

uniform vec4 color;
uniform sampler2D textureSample;

layout (location = 0) out vec4 fragColor;

void main()
{
	fragColor = color * texture(textureSample, texVarying);
}
