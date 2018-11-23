#include "renderer.h"
#include "utilities.h"

#ifdef WINDOWS
#include "SDL_opengl.h"
#else
#include <SDL2/SDL_opengl.h>
#endif

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
	renderer->window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, videoFlags | SDL_WINDOW_OPENGL);
	
	SDL_GLContext glContext;
	if (renderer->window == NULL || (glContext = SDL_GL_CreateContext(renderer->window)) == NULL)
	{
		if (!fsaa)
		{
			zgPrint("Couldn't create SDL window with resolution %ix%i: %e", windowWidth, windowHeight);
			exit(4);
		}
		else
		{
			// Try creating SDL window and opengl context again except without anti-aliasing this time
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
			
			if (renderer->window != NULL)
			{
				SDL_DestroyWindow(renderer->window);
			}
			
			renderer->window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, videoFlags | SDL_WINDOW_OPENGL);
			
			if (renderer->window == NULL)
			{
				zgPrint("Couldn't create SDL window second time with resolution %ix%i: %e", windowWidth, windowHeight);
				exit(5);
			}
			
			glContext = SDL_GL_CreateContext(renderer->window);
			if (glContext == NULL)
			{
				zgPrint("Couldn't create GL context after creating window second time with resolution %ix%i: %e", windowWidth, windowHeight);
				exit(5);
			}
		}
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

void clearColorAndDepthBuffers(Renderer *renderer)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

static GLenum glTypeFromIndicesType(uint8_t type)
{
	switch (type)
	{
		case RENDERER_INT8_TYPE:
			return GL_UNSIGNED_BYTE;
		case RENDERER_INT16_TYPE:
			return GL_UNSIGNED_SHORT;
	}
	return 0;
}

static GLenum glTypeFromCoordinatesType(uint8_t type)
{
	switch (type)
	{
		case RENDERER_INT16_TYPE:
			return GL_SHORT;
		case RENDERER_FLOAT_TYPE:
			return GL_FLOAT;
	}
	return 0;
}

static GLenum glModeFromMode(uint8_t mode)
{
	switch (mode)
	{
		case RENDERER_TRIANGLE_MODE:
			return GL_TRIANGLES;
		case RENDERER_TRIANGLE_STRIP_MODE:
			return GL_TRIANGLE_STRIP;
	}
	return 0;
}

static void beginDrawingVertices(mat4_t modelViewMatrix, const float *vertices, uint8_t vertexSize, color4_t color, uint8_t options)
{
	glLoadMatrixf(&modelViewMatrix.m00);
	
	SDL_bool disableDepthTest = (options & RENDERER_OPTION_DISABLE_DEPTH_TEST) != 0;
	if (disableDepthTest)
	{
		glDisable(GL_DEPTH_TEST);
	}
	
	SDL_bool blendingAlpha = (options & RENDERER_OPTION_BLENDING_ALPHA) != 0;
	SDL_bool blendingOneMinusAlpha = (options & RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA) != 0;
	SDL_bool blending = (blendingAlpha || blendingOneMinusAlpha);
	if (blending)
	{
		glEnable(GL_BLEND);
		
		if (blendingAlpha)
		{
			glBlendFunc(GL_SRC_ALPHA, GL_SRC_ALPHA);
		}
		else /* if (blendingOneMinusAlpha) */
		{
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
	}
	
	glColor4fv(&color.red);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	
	glVertexPointer(vertexSize, GL_FLOAT, 0, vertices);
}

static void endDrawingVertices(uint8_t options)
{
	glDisableClientState(GL_VERTEX_ARRAY);
	
	SDL_bool blendingAlpha = (options & RENDERER_OPTION_BLENDING_ALPHA) != 0;
	SDL_bool blendingOneMinusAlpha = (options & RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA) != 0;
	SDL_bool blending = (blendingAlpha || blendingOneMinusAlpha);
	if (blending)
	{
		glDisable(GL_BLEND);
	}
	
	SDL_bool disableDepthTest = (options & RENDERER_OPTION_DISABLE_DEPTH_TEST) != 0;
	if (disableDepthTest)
	{
		glEnable(GL_DEPTH_TEST);
	}
}

void drawVerticesFromIndices(Renderer *renderer, mat4_t modelViewMatrix, uint8_t mode, const float *vertices, uint8_t vertexSize, const void *indices, uint8_t indicesType, uint32_t indicesCount, color4_t color, uint8_t options)
{
	beginDrawingVertices(modelViewMatrix, vertices, vertexSize, color, options);
	
	glDrawElements(glModeFromMode(mode), indicesCount, glTypeFromIndicesType(indicesType), indices);
	
	endDrawingVertices(options);
}

void drawVertices(Renderer *renderer, mat4_t modelViewMatrix, uint8_t mode, const float *vertices, uint8_t vertexSize, uint32_t vertexCount, color4_t color, uint8_t options)
{
	beginDrawingVertices(modelViewMatrix, vertices, vertexSize, color, options);
	
	glDrawArrays(glModeFromMode(mode), 0, vertexCount);
	
	endDrawingVertices(options);
}

static void beginDrawingTexture(mat4_t modelViewMatrix, uint32_t texture, const void *textureCoordinates, uint8_t textureCoordinatesType, const float *vertices, uint8_t vertexSize, color4_t color, uint8_t options)
{
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glBindTexture(GL_TEXTURE_2D, texture);
	
	beginDrawingVertices(modelViewMatrix, vertices, vertexSize, color, options);
	
	glTexCoordPointer(2, glTypeFromCoordinatesType(textureCoordinatesType), 0, textureCoordinates);
}

static void endDrawingTexture(uint8_t options)
{
	endDrawingVertices(options);
	
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
}

void drawTextureWithVerticesFromIndices(Renderer *renderer, mat4_t modelViewMatrix, uint32_t texture, uint8_t mode, const float *vertices, uint8_t vertexSize, const void *textureCoordinates, uint8_t textureCoordinatesType, const void *indices, uint8_t indicesType, uint32_t indicesCount, color4_t color, uint8_t options)
{
	beginDrawingTexture(modelViewMatrix, texture, textureCoordinates, textureCoordinatesType, vertices, vertexSize, color, options);
	
	glDrawElements(glModeFromMode(mode), indicesCount, glTypeFromIndicesType(indicesType), indices);
	
	endDrawingTexture(options);
}

void drawTextureWithVertices(Renderer *renderer, mat4_t modelViewMatrix, uint32_t texture, uint8_t mode, const float *vertices, uint8_t vertexSize, const void *textureCoordinates, uint8_t textureCoordinatesType, uint32_t vertexCount, color4_t color, uint8_t options)
{
	beginDrawingTexture(modelViewMatrix, texture, textureCoordinates, textureCoordinatesType, vertices, vertexSize, color, options);
	
	glDrawArrays(glModeFromMode(mode), 0, vertexCount);
	
	endDrawingTexture(options);
}
