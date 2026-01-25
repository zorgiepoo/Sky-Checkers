out vec4 fragColor;

in vec2 texVarying;

uniform vec4 color;
uniform sampler2D textureSample;

void main()
{
	fragColor = color * texture(textureSample, texVarying);
}
