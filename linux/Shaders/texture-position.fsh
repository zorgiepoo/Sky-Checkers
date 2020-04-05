#if __VERSION__ < 130
#define in varying
#define texture texture2D
#define fragColor gl_FragColor
#else
out vec4 fragColor;
#endif

in vec2 texVarying;

uniform vec4 color;
uniform sampler2D textureSample;

void main()
{
	fragColor = color * texture(textureSample, texVarying);
}
