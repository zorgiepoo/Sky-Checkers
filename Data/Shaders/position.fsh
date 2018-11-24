#if __VERSION__ < 130
#define fragColor gl_FragColor
#else
out vec4 fragColor;
#endif

uniform vec4 color;

void main()
{
	fragColor = color;
}
