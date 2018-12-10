out vec4 fragColor;

in vec2 texVarying;

flat in int instanceID;

uniform vec4 colors[64];
uniform uint textureIndices[64];
uniform sampler2DArray textureSamples;

void main()
{
	fragColor = colors[instanceID] * texture(textureSamples, vec3(texVarying, textureIndices[instanceID]));
}
