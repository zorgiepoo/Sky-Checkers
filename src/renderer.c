#include "renderer.h"
#include "utilities.h"

void createRenderer(Renderer *renderer, int32_t windowWidth, int32_t windowHeight, uint32_t videoFlags, SDL_bool vsync, SDL_bool fsaa)
{
	// Choose OpenGL 2.1 for now which almost anything should support
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	
	// FSAA
	if (fsaa)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);
	}
	
	// VSYNC
	SDL_GL_SetSwapInterval(vsync);
	
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	
#ifndef _DEBUG
	SDL_ShowCursor(SDL_DISABLE);
#endif
	
#ifndef MAC_OS_X
	const char *windowTitle = "SkyCheckers";
#else
	const char *windowTitle = "";
#endif
	renderer->window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, videoFlags);
	
	if (renderer->window == NULL)
	{
		if (!fsaa)
		{
			zgPrint("Couldn't create SDL window with resolution %ix%i: %e", windowWidth, windowHeight);
			exit(4);
		}
		else
		{
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
			
			renderer->window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, videoFlags);
			
			if (renderer->window == NULL)
			{
				zgPrint("Couldn't create SDL window with fsaa off with resolution %ix%i: %e", windowWidth, windowHeight);
				exit(6);
			}
		}
	}
	
	SDL_GLContext glContext = SDL_GL_CreateContext(renderer->window);
	if (glContext == NULL)
	{
		zgPrint("Couldn't create OpenGL context: %e");
		exit(5);
	}
	
	if (SDL_GL_MakeCurrent(renderer->window, glContext) != 0)
	{
		zgPrint("Couldn't make OpenGL context current: %e");
		exit(9);
	}
	
	int value;
	SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &value);
	renderer->fsaa = (value != 0);
	
	value = SDL_GL_GetSwapInterval();
	renderer->vsync = (value != 0);
	
	SDL_GetWindowSize(renderer->window, &renderer->windowWidth, &renderer->windowHeight);
	SDL_GL_GetDrawableSize(renderer->window, &renderer->screenWidth, &renderer->screenHeight);
	
	// Set the projection
	glViewport(0, 0, renderer->screenWidth, renderer->screenHeight);
	
	glMatrixMode(GL_PROJECTION);
	
	// The aspect ratio is not quite correct, which is a mistake I made a long time ago that is too troubling to fix properly
	renderer->projectionMatrix = m4_perspective(45.0f, (float)(renderer->screenWidth / renderer->screenHeight), 10.0f, 300.0f);
	glLoadMatrixf(&renderer->projectionMatrix.m00);
	
	glMatrixMode(GL_MODELVIEW);
	
	// OpenGL Initialization
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
}

void swapBuffers(Renderer *renderer)
{
	SDL_GL_SwapWindow(renderer->window);
}

uint32_t textureFromPixelData(Renderer *renderer, const void *pixels, int32_t width, int32_t height)
{
	GLuint tex = 0;
	
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D,
				 0,
				 GL_RGBA,
				 width, height,
				 0,
				 GL_RGBA,
				 GL_UNSIGNED_BYTE,
				 pixels);
	
	return tex;
}

// This function is derived from same place as surfaceToGLTexture() code is
static int power_of_two(int input)
{
	int value = 1;
	
	while ( value < input )
	{
		value <<= 1;
	}
	
	return value;
}

// The code from this function is taken from https://www.opengl.org/discussion_boards/showthread.php/163677-SDL_image-Opengl
// which is derived from SDL 1.2 source code which is licensed under LGPL:
// https://github.com/klange/SDL/blob/master/test/testgl.c
static void surfaceToTexture(Renderer *renderer, SDL_Surface *surface, GLuint *tex)
{
	int w, h;
	SDL_Surface *image;
	SDL_Rect area;
	
	/* Use the surface width and height expanded to powers of 2 */
	w = power_of_two(surface->w);
	h = power_of_two(surface->h);
	
	image = SDL_CreateRGBSurface(
								 SDL_SWSURFACE,
								 w, h,
								 32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN /* OpenGL RGBA masks */
								 0x000000FF,
								 0x0000FF00,
								 0x00FF0000,
								 0xFF000000
#else
								 0xFF000000,
								 0x00FF0000,
								 0x0000FF00,
								 0x000000FF
#endif
								 );
	if ( image == NULL )
	{
		return;
	}
	
	// Set alpha property to max
	SDL_SetSurfaceAlphaMod(surface, 255);
	
	/* Copy the surface into the GL texture image */
	area.x = 0;
	area.y = 0;
	area.w = surface->w;
	area.h = surface->h;
	SDL_BlitSurface(surface, &area, image, &area);
	
	/* Create an OpenGL texture for the image */
	*tex = textureFromPixelData(renderer, image->pixels, w, h);
	
	SDL_FreeSurface(image); /* No longer needed */
}

void loadTexture(Renderer *renderer, const char *filePath, uint32_t *tex)
{
	SDL_Surface *texImage;
	
	texImage = IMG_Load(filePath);
	
	if (texImage == NULL)
	{
		printf("Couldn't load texture: %s\n", filePath);
		SDL_Quit();
	}
	
	surfaceToTexture(renderer, texImage, tex);
	
	SDL_FreeSurface(texImage);
}
