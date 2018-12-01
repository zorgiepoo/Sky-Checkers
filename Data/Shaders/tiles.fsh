out vec4 fragColor;

in vec2 texVarying;

flat in int instanceID;

uniform vec4 colors[64];
uniform uint textureIndices[64];
uniform sampler2D textureSamples[2];

void main()
{
	fragColor = colors[instanceID] * texture(textureSamples[textureIndices[instanceID]], texVarying);
}
