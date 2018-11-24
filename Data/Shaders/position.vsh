#version 330

layout (location = 0) in vec4 position;

uniform mat4 modelViewProjectionMatrix;

void main()
{
	gl_Position = modelViewProjectionMatrix * position;
}
