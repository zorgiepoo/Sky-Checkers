#if __VERSION__ < 130
#define in attribute
#define out varying
#endif

in vec4 position;
in vec2 textureCoordIn;

out vec2 texVarying;

uniform mat4 modelViewProjectionMatrix;

void main()
{
	texVarying = textureCoordIn;
	gl_Position = modelViewProjectionMatrix * position;
}
