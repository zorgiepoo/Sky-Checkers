out vec4 fragColor;

in vec2 texVarying;

flat in int instanceID;

uniform vec4 colors[64];
uniform sampler2D textureSample;

void main()
{
	fragColor = colors[instanceID] * texture(textureSample, texVarying);
}
