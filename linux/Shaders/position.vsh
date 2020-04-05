#if __VERSION__ < 130
#define in attribute
#endif

in vec4 position;

uniform mat4 modelViewProjectionMatrix;

void main()
{
	gl_Position = modelViewProjectionMatrix * position;
}
