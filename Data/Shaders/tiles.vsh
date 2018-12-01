in vec4 position;
in vec2 textureCoordIn;

uniform mat4 modelViewProjectionMatrices[64];

out vec2 texVarying;
flat out int instanceID;

void main()
{
	texVarying = textureCoordIn;
	instanceID = gl_InstanceID;
	gl_Position = modelViewProjectionMatrices[gl_InstanceID] * position;
}
