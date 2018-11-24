#include "renderer.h"
#include "utilities.h"

// On platforms besides macOS I will need to use something like GLEW
#ifdef WINDOWS
#include <GL/glew.h>
#include "SDL_opengl.h"
#endif

#ifdef linux
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#endif

#ifdef MAC_OS_X
#include <OpenGL/gl3.h>
#endif

#define VERTEX_ATTRIBUTE 0
#define TEXTURE_ATTRIBUTE 1

static SDL_bool compileShader(GLuint *shader, GLenum type, const char *filepath)
{
	GLint status;
	
	const char *baseDirectory = SDL_GetBasePath();
	size_t baseDirectoryLength = strlen(baseDirectory);
	char pathBuffer[4096 + 1] = {0};
	
	if (baseDirectoryLength > 4096)
	{
		zgPrint("Shader can't compile because base directory is too big");
		return SDL_FALSE;
	}
	
	strcpy(pathBuffer, baseDirectory);
	strncpy(pathBuffer + baseDirectoryLength, filepath, 4096 - baseDirectoryLength);
	
	FILE *sourceFile = fopen(pathBuffer, "r");
	if (sourceFile == NULL)
	{
		zgPrint("Shader doesn't exist at: %s", pathBuffer);
		return SDL_FALSE;
	}
	
	fseek(sourceFile, 0, SEEK_END);
	GLint fileSize = (GLint)ftell(sourceFile);
	fseek(sourceFile, 0, SEEK_SET);
	
	GLchar *source = (GLchar *)malloc(fileSize);
	fread(source, fileSize, 1, sourceFile);
	
	fclose(sourceFile);
	
	*shader = glCreateShader(type);
	glShaderSource(*shader, 1, (const char **)&source, &fileSize);
	
	glCompileShader(*shader);
	
	free(source);
	
#ifdef _DEBUG
	GLint logLength;
	glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0)
	{
		GLchar *log = (GLchar *)malloc(logLength);
		glGetShaderInfoLog(*shader, logLength, &logLength, log);
		zgPrint("Shader compiler log:\n%s", log);
		free(log);
	}
#endif
	
	glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
	if (status == 0)
	{
		glDeleteShader(*shader);
		return SDL_FALSE;
	}
	
	return SDL_TRUE;
}

static SDL_bool linkProgram(GLuint prog)
{
	GLint status;
	glLinkProgram(prog);
	
#ifdef _DEBUG
	GLint logLength;
	glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0)
	{
		GLchar *log = (GLchar *)malloc(logLength);
		glGetProgramInfoLog(prog, logLength, &logLength, log);
		zgPrint("Program link log:\n%s", log);
		free(log);
	}
#endif
	
	glGetProgramiv(prog, GL_LINK_STATUS, &status);
	if (status == 0)
	{
		return SDL_FALSE;
	}
	
	return SDL_TRUE;
}

static void compileAndLinkShader(Shader *shader, const char *vertexShaderPath, const char *fragmentShaderPath, SDL_bool textured)
{
	// Create a pair of shaders
	GLuint vertexShader = 0;
	if (!compileShader(&vertexShader, GL_VERTEX_SHADER, vertexShaderPath))
	{
		zgPrint("Error: Failed to compile vertex shader: %s..\n", vertexShaderPath);
		exit(2);
	}
	
	GLuint fragmentShader = 0;
	if (!compileShader(&fragmentShader, GL_FRAGMENT_SHADER, fragmentShaderPath))
	{
		zgPrint("Error: Failed to compile fragment shader: %s..\n", fragmentShaderPath);
		exit(2);
	}
	
	GLuint shaderProgram = glCreateProgram();
	if (shaderProgram == 0)
	{
		zgPrint("Failed to create a shader program.. Odd.");
		exit(1);
	}
	
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	
	glBindAttribLocation(shaderProgram, VERTEX_ATTRIBUTE, "position");
	if (textured)
	{
		glBindAttribLocation(shaderProgram, TEXTURE_ATTRIBUTE, "textureCoordIn");
	}
	
	if (!linkProgram(shaderProgram))
	{
		zgPrint("Failed to link shader program");
		exit(2);
	}
	
	GLint modelViewProjectionMatrixUniformLocation = glGetUniformLocation(shaderProgram, "modelViewProjectionMatrix");
	if (modelViewProjectionMatrixUniformLocation == -1)
	{
		zgPrint("Failed to find MVP uniform");
		exit(1);
	}
	shader->modelViewProjectionMatrixUniformLocation = modelViewProjectionMatrixUniformLocation;
	
	GLint colorUniformLocation = glGetUniformLocation(shaderProgram, "color");
	if (colorUniformLocation == -1)
	{
		zgPrint("Failed to find color uniform");
		exit(1);
	}
	shader->colorUniformLocation = colorUniformLocation;
	
	if (textured)
	{
		GLint textureUniformLocation = glGetUniformLocation(shaderProgram, "textureSample");
		if (textureUniformLocation == -1)
		{
			zgPrint("Failed to find textureSample uniform");
			exit(1);
		}
		shader->textureUniformLocation = textureUniformLocation;
	}
	
	shader->program = shaderProgram;
	
	glDetachShader(shaderProgram, vertexShader);
	glDeleteShader(vertexShader);
	
	glDetachShader(shaderProgram, fragmentShader);
	glDeleteShader(fragmentShader);
}

void createRenderer(Renderer *renderer, int32_t windowWidth, int32_t windowHeight, uint32_t videoFlags, SDL_bool vsync, SDL_bool fsaa)
{
	// Target OpenGL 3.3 which any GPU from around 2007 or so should support
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	
	// Anti aliasing
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

#ifndef MAC_OS_X
	glewExperimental = GL_TRUE;
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK)
	{
		zgPrint("Failed to initialize GLEW: %s", glewGetErrorString(glewError));
		exit(6);
	}
#endif
	
	int value;
	SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &value);
	renderer->fsaa = (value != 0);
	
	value = SDL_GL_GetSwapInterval();
	renderer->vsync = (value != 0);
	
	SDL_GetWindowSize(renderer->window, &renderer->windowWidth, &renderer->windowHeight);
	SDL_GL_GetDrawableSize(renderer->window, &renderer->screenWidth, &renderer->screenHeight);
	
	// Set the projection
	glViewport(0, 0, renderer->screenWidth, renderer->screenHeight);
	
	// The aspect ratio is not quite correct, which is a mistake I made a long time ago that is too troubling to fix properly
	renderer->projectionMatrix = m4_perspective(45.0f, (float)(renderer->screenWidth / renderer->screenHeight), 10.0f, 300.0f);
	
	// OpenGL Initialization
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	
	compileAndLinkShader(&renderer->positionShader, "Data/Shaders/position.vsh", "Data/Shaders/position.fsh", SDL_FALSE);
	
	compileAndLinkShader(&renderer->positionTextureShader, "Data/Shaders/texture-position.vsh", "Data/Shaders/texture-position.fsh", SDL_TRUE);
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

uint32_t createVertexBufferObject(const void *data, uint32_t size)
{
	GLuint buffer = 0;
	glGenBuffers(1, &buffer);
	
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	return buffer;
}

uint32_t createVertexArrayObject(const void *vertices, uint32_t verticesSize, uint8_t vertexComponents)
{
	GLuint vertexArray = 0;
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);
	
	GLuint vertexBuffer = 0;
	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, verticesSize, vertices, GL_STATIC_DRAW);
	
	const GLuint vertexAttributeIndex = 0;
	glEnableVertexAttribArray(vertexAttributeIndex);
	glVertexAttribPointer(vertexAttributeIndex, vertexComponents, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
	
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	return vertexArray;
}

uint32_t createVertexAndTextureCoordinateArrayObject(const void *verticesAndTextureCoordinates, uint32_t verticesSize, uint8_t vertexComponents, uint32_t textureCoordinatesSize, uint8_t textureCoordinateType)
{
	GLuint vertexArray = 0;
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);
	
	GLuint buffer = 0;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, verticesSize + textureCoordinatesSize, verticesAndTextureCoordinates, GL_STATIC_DRAW);
	
	glEnableVertexAttribArray(VERTEX_ATTRIBUTE);
	glVertexAttribPointer(VERTEX_ATTRIBUTE, vertexComponents, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
	
	glEnableVertexAttribArray(TEXTURE_ATTRIBUTE);
	glVertexAttribPointer(TEXTURE_ATTRIBUTE, 2, glTypeFromCoordinatesType(textureCoordinateType), GL_FALSE, 0, (GLvoid *)(uintptr_t)verticesSize);
	
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	return vertexArray;
}

static void beginDrawingVertices(Shader *shader, mat4_t modelViewMatrix, mat4_t projectionMatrix, uint32_t vertexArrayObject, color4_t color, uint8_t options)
{
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
	
	glBindVertexArray(vertexArrayObject);
	
	glUseProgram(shader->program);
	
	mat4_t modelViewProjectionMatrix = m4_mul(projectionMatrix, modelViewMatrix);
	glUniformMatrix4fv(shader->modelViewProjectionMatrixUniformLocation, 1, GL_FALSE, &modelViewProjectionMatrix.m00);
	glUniform4f(shader->colorUniformLocation, color.red, color.green, color.blue, color.alpha);
}

static void endDrawingVerticesAndTextures(uint8_t options)
{
	glBindVertexArray(0);
	
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

void drawVertices(Renderer *renderer, mat4_t modelViewMatrix, uint8_t mode, uint32_t vertexArrayObject, uint32_t vertexCount, color4_t color, uint8_t options)
{
	beginDrawingVertices(&renderer->positionShader, modelViewMatrix, renderer->projectionMatrix, vertexArrayObject, color, options);
	
	glDrawArrays(glModeFromMode(mode), 0, vertexCount);
	
	endDrawingVerticesAndTextures(options);
}

void drawVerticesFromIndices(Renderer *renderer, mat4_t modelViewMatrix, uint8_t mode, uint32_t vertexArrayObject, uint32_t indicesBufferObject, uint8_t indicesType, uint32_t indicesCount, color4_t color, uint8_t options)
{
	beginDrawingVertices(&renderer->positionShader, modelViewMatrix, renderer->projectionMatrix, vertexArrayObject, color, options);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBufferObject);
	
	glDrawElements(glModeFromMode(mode), indicesCount, glTypeFromIndicesType(indicesType), NULL);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	endDrawingVerticesAndTextures(options);
}

static void beginDrawingTexture(Shader *shader, mat4_t modelViewMatrix, mat4_t projectionMatrix, uint32_t texture, uint32_t vertexAndTextureArrayObject, color4_t color, uint8_t options)
{
	beginDrawingVertices(shader, modelViewMatrix, projectionMatrix, vertexAndTextureArrayObject, color, options);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(shader->textureUniformLocation, 0);
}

void drawTextureWithVertices(Renderer *renderer, mat4_t modelViewMatrix, uint32_t texture, uint8_t mode, uint32_t vertexAndTextureArrayObject, uint32_t vertexCount, color4_t color, uint8_t options)
{
	beginDrawingTexture(&renderer->positionTextureShader, modelViewMatrix, renderer->projectionMatrix, texture, vertexAndTextureArrayObject, color, options);
	
	glDrawArrays(glModeFromMode(mode), 0, vertexCount);
	
	endDrawingVerticesAndTextures(options);
}

void drawTextureWithVerticesFromIndices(Renderer *renderer, mat4_t modelViewMatrix, uint32_t texture, uint8_t mode, uint32_t vertexAndTextureArrayObject, uint32_t indicesBufferObject, uint8_t indicesType, uint32_t indicesCount, color4_t color, uint8_t options)
{
	beginDrawingTexture(&renderer->positionTextureShader, modelViewMatrix, renderer->projectionMatrix, texture, vertexAndTextureArrayObject, color, options);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBufferObject);
	
	glDrawElements(glModeFromMode(mode), indicesCount, glTypeFromIndicesType(indicesType), NULL);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	endDrawingVerticesAndTextures(options);
}
