out vec4 fragColor;

in vec2 texVarying;

flat in int instanceID;

uniform vec4 colors[16];
uniform sampler2D textureSamples[16];

void main()
{
	fragColor = colors[instanceID] * texture(textureSamples[instanceID], texVarying);
}
