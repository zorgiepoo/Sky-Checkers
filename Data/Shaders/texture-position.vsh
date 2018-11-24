#version 330

layout (location = 0) in vec4 position;
layout (location = 1) in vec2 textureCoordIn;

out lowp vec2 texVarying;

uniform mat4 modelViewProjectionMatrix;

void main()
{
	texVarying = textureCoordIn;
	gl_Position = modelViewProjectionMatrix * position;
}
