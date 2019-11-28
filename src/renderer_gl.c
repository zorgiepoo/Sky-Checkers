/*
 * Copyright 2018 Mayur Pawashe
 * https://zgcoder.net
 
 * This file is part of skycheckers.
 * skycheckers is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 
 * skycheckers is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with skycheckers.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "renderer_gl.h"

#include "platforms.h"
#include "renderer_projection.h"
#include "texture.h"
#include "quit.h"
#include "window.h"
#include "sdl.h"

#ifdef WINDOWS
#include <GL/glew.h>
#include "SDL_opengl.h"
#endif

#ifdef linux
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#endif

#define VERTEX_ATTRIBUTE 0
#define TEXTURE_ATTRIBUTE 1

#define GLSL_VERSION_410 410
#define GLSL_VERSION_330 330
#define GLSL_VERSION_120 120

static void updateViewport_gl(Renderer *renderer, int32_t windowWidth, int32_t windowHeight);

void renderFrame_gl(Renderer *renderer, void (*drawFunc)(Renderer *));

TextureObject textureFromPixelData_gl(Renderer *renderer, const void *pixels, int32_t width, int32_t height, PixelFormat pixelFormat);

void deleteTexture_gl(Renderer *renderer, TextureObject texture);

BufferObject createIndexBufferObject_gl(Renderer *renderer, const void *data, uint32_t size);

BufferArrayObject createVertexArrayObject_gl(Renderer *renderer, const void *vertices, uint32_t verticesSize);

BufferArrayObject createVertexAndTextureCoordinateArrayObject_gl(Renderer *renderer, const void *verticesAndTextureCoordinates, uint32_t verticesSize, uint32_t textureCoordinatesSize);

void drawVertices_gl(Renderer *renderer, float *modelViewProjectionMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options);

void drawVerticesFromIndices_gl(Renderer *renderer, float *modelViewProjectionMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options);

void drawTextureWithVertices_gl(Renderer *renderer, float *modelViewProjectionMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options);

void drawTextureWithVerticesFromIndices_gl(Renderer *renderer, float *modelViewProjectionMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options);

static bool compileShader(GLuint *shader, uint16_t glslVersion, GLenum type, const char *filepath)
{
	GLint status;
	
	FILE *sourceFile = fopen(filepath, "r");
	if (sourceFile == NULL)
	{
		fprintf(stderr, "Shader doesn't exist at: %s\n", filepath);
		return false;
	}
	
	fseek(sourceFile, 0, SEEK_END);
	GLint fileSize = (GLint)ftell(sourceFile);
	fseek(sourceFile, 0, SEEK_SET);
	
	GLchar *source = (GLchar *)malloc(fileSize);
	if (fread(source, fileSize, 1, sourceFile) < 1)
	{
		fprintf(stderr, "Failed to fread entire contents of shader: %s\n", filepath);
		fclose(sourceFile);
		return false;
	}
	
	fclose(sourceFile);
	
	*shader = glCreateShader(type);
	
	GLchar versionLine[256] = {0};
	snprintf(versionLine, sizeof(versionLine) - 1, "#version %u\n", glslVersion);
	glShaderSource(*shader, 2, (const GLchar *[]){versionLine, source}, (GLint []){(GLint)strlen(versionLine), fileSize});
	
	glCompileShader(*shader);
	
	free(source);
	
#ifdef _DEBUG
	GLint logLength;
	glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0)
	{
		GLchar *log = (GLchar *)malloc(logLength);
		glGetShaderInfoLog(*shader, logLength, &logLength, log);
		fprintf(stderr, "Shader compiler log:\n%s\n", log);
		free(log);
	}
#endif
	
	glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
	if (status == 0)
	{
		glDeleteShader(*shader);
		return false;
	}
	
	return true;
}

static bool linkProgram(GLuint prog)
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
		fprintf(stderr, "Program link log:\n%s\n", log);
		free(log);
	}
#endif
	
	glGetProgramiv(prog, GL_LINK_STATUS, &status);
	if (status == 0)
	{
		return false;
	}
	
	return true;
}

static void compileAndLinkShader(Shader_gl *shader, uint16_t glslVersion, const char *vertexShaderPath, const char *fragmentShaderPath, bool textured, const char *modelViewProjectionUniform, const char *colorUniform, const char *textureSampleUniform)
{
	// Create a pair of shaders
	GLuint vertexShader = 0;
	if (!compileShader(&vertexShader, glslVersion, GL_VERTEX_SHADER, vertexShaderPath))
	{
		fprintf(stderr, "Error: Failed to compile vertex shader: %s..\n", vertexShaderPath);
		ZGQuit();
	}
	
	GLuint fragmentShader = 0;
	if (!compileShader(&fragmentShader, glslVersion, GL_FRAGMENT_SHADER, fragmentShaderPath))
	{
		fprintf(stderr, "Error: Failed to compile fragment shader: %s..\n", fragmentShaderPath);
		ZGQuit();
	}
	
	GLuint shaderProgram = glCreateProgram();
	if (shaderProgram == 0)
	{
		fprintf(stderr, "Failed to create a shader program.. Odd.\n");
		ZGQuit();
	}
	
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	
	glBindAttribLocation(shaderProgram, VERTEX_ATTRIBUTE, "position");
	if (textured)
	{
		glBindAttribLocation(shaderProgram, TEXTURE_ATTRIBUTE, "textureCoordIn");
	}
	
	if (glslVersion < 130)
	{
		glBindFragDataLocation(shaderProgram, 0, "gl_FragColor");
	}
	else
	{
		glBindFragDataLocation(shaderProgram, 0, "fragColor");
	}
	
	if (!linkProgram(shaderProgram))
	{
		fprintf(stderr, "Failed to link shader program\n");
		ZGQuit();
	}
	
	GLint modelViewProjectionMatrixUniformLocation = glGetUniformLocation(shaderProgram, modelViewProjectionUniform);
	if (modelViewProjectionMatrixUniformLocation == -1)
	{
		fprintf(stderr, "Failed to find %s uniform\n", modelViewProjectionUniform);
		ZGQuit();
	}
	shader->modelViewProjectionMatrixUniformLocation = modelViewProjectionMatrixUniformLocation;
	
	GLint colorUniformLocation = glGetUniformLocation(shaderProgram, colorUniform);
	if (colorUniformLocation == -1)
	{
		fprintf(stderr, "Failed to find %s uniform\n", colorUniform);
		ZGQuit();
	}
	shader->colorUniformLocation = colorUniformLocation;
	
	if (textured)
	{
		GLint textureUniformLocation = glGetUniformLocation(shaderProgram, textureSampleUniform);
		if (textureUniformLocation == -1)
		{
			fprintf(stderr, "Failed to find %s uniform\n", textureSampleUniform);
			ZGQuit();
		}
		shader->textureUniformLocation = textureUniformLocation;
	}
	
	shader->program = shaderProgram;
	
	glDetachShader(shaderProgram, vertexShader);
	glDeleteShader(vertexShader);
	
	glDetachShader(shaderProgram, fragmentShader);
	glDeleteShader(fragmentShader);
}

static bool createOpenGLContext(ZGWindow **window, SDL_GLContext *glContext, uint16_t glslVersion, const char *windowTitle, int32_t windowWidth, int32_t windowHeight, bool *fullscreenFlag, bool fsaa)
{
	switch (glslVersion)
	{
		case GLSL_VERSION_410:
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
			break;
		case GLSL_VERSION_330:
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
			break;
		case GLSL_VERSION_120:
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
			break;
		default:
			fprintf(stderr, "Invalid glsl version passed..\n");
			ZGQuit();
	}
	
	// Anti aliasing
	if (fsaa)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		// Platforms other than macOS should prefer the non-retina sample count I suppose
		// macOS should prefer the metal renderer which does the right thing
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, MSAA_PREFERRED_NONRETINA_SAMPLE_COUNT);
	}
	
	*window = ZGCreateWindow(windowTitle, windowWidth, windowHeight, fullscreenFlag);
	
	if (*window == NULL || (*glContext = SDL_GL_CreateContext(*window)) == NULL)
	{
		if (!fsaa)
		{
			if (*window != NULL)
			{
				ZGDestroyWindow(*window);
			}
			return false;
		}
		else
		{
			if (*window != NULL)
			{
				ZGDestroyWindow(*window);
			}
			
			// Try creating SDL window and opengl context again except without anti-aliasing
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
			
			*window = ZGCreateWindow(windowTitle, windowWidth, windowHeight, fullscreenFlag);
			
			if (*window == NULL)
			{
				return false;
			}
			
			*glContext = SDL_GL_CreateContext(*window);
			if (*glContext == NULL)
			{
				ZGDestroyWindow(*window);
				
				return false;
			}
		}
	}
	return true;
}

static void updateViewport_gl(Renderer *renderer, int32_t windowWidth, int32_t windowHeight)
{
	if (!ZGWindowIsFullscreen(renderer->window) && !renderer->fullscreen)
	{
		renderer->windowWidth = windowWidth;
		renderer->windowHeight = windowHeight;
	}

	SDL_GL_GetDrawableSize(renderer->window, &renderer->drawableWidth, &renderer->drawableHeight);
	
	glViewport(0, 0, renderer->drawableWidth, renderer->drawableHeight);
	
	updateGLProjectionMatrix(renderer);
}

void createRenderer_gl(Renderer *renderer, const char *windowTitle, int32_t windowWidth, int32_t windowHeight, bool fullscreen, bool vsync, bool fsaa)
{
	renderer->windowWidth = windowWidth;
	renderer->windowHeight = windowHeight;
	renderer->fullscreen = fullscreen;
	
	// Buffer sizes
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	
	uint16_t glslVersion = GLSL_VERSION_410;
	SDL_GLContext glContext = NULL;
	if (!createOpenGLContext(&renderer->window, &glContext, glslVersion, windowTitle, windowWidth, windowHeight, &renderer->fullscreen, fsaa))
	{
		glslVersion = GLSL_VERSION_330;
		if (!createOpenGLContext(&renderer->window, &glContext, glslVersion, windowTitle, windowWidth, windowHeight, &renderer->fullscreen, fsaa))
		{
			glslVersion = GLSL_VERSION_120;
			if (!createOpenGLContext(&renderer->window, &glContext, glslVersion, windowTitle, windowWidth, windowHeight, &renderer->fullscreen, fsaa))
			{
				fprintf(stderr, "Failed to create OpenGL context with even glsl version %d\n", glslVersion);
				ZGQuit();
			}
		}
	}
	
	if (SDL_GL_MakeCurrent(renderer->window, glContext) != 0)
	{
		fprintf(stderr, "Couldn't make OpenGL context current: %s\n", SDL_GetError());
		ZGQuit();
	}
	
	// VSYNC
	SDL_GL_SetSwapInterval(!!vsync);
	
	glewExperimental = GL_TRUE;
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK)
	{
		fprintf(stderr, "Failed to initialize GLEW: %s\n", glewGetErrorString(glewError));
		ZGQuit();
	}
	
	int value;
	SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &value);
	renderer->fsaa = (value != 0);
	
	if (renderer->fsaa)
	{
		SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &value);
		renderer->sampleCount = value;
	}
	else
	{
		renderer->sampleCount = 0;
	}
	
	value = SDL_GL_GetSwapInterval();
	renderer->vsync = (value != 0);
	
	updateViewport_gl(renderer, renderer->windowWidth, renderer->windowHeight);
	
	// OpenGL Initialization
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f);
	
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	
	if (renderer->fsaa)
	{
		glEnable(GL_MULTISAMPLE);
	}
	
	compileAndLinkShader(&renderer->glPositionShader, glslVersion, "Data/Shaders/position.vsh", "Data/Shaders/position.fsh", false, "modelViewProjectionMatrix", "color", NULL);
	
	compileAndLinkShader(&renderer->glPositionTextureShader, glslVersion, "Data/Shaders/texture-position.vsh", "Data/Shaders/texture-position.fsh", true, "modelViewProjectionMatrix", "color", "textureSample");
	
	renderer->updateViewportPtr = updateViewport_gl;
	renderer->renderFramePtr = renderFrame_gl;
	renderer->textureFromPixelDataPtr = textureFromPixelData_gl;
	renderer->deleteTexturePtr = deleteTexture_gl;
	renderer->createIndexBufferObjectPtr = createIndexBufferObject_gl;
	renderer->createVertexArrayObjectPtr = createVertexArrayObject_gl;
	renderer->createVertexAndTextureCoordinateArrayObjectPtr = createVertexAndTextureCoordinateArrayObject_gl;
	renderer->drawVerticesPtr = drawVertices_gl;
	renderer->drawVerticesFromIndicesPtr = drawVerticesFromIndices_gl;
	renderer->drawTextureWithVerticesPtr = drawTextureWithVertices_gl;
	renderer->drawTextureWithVerticesFromIndicesPtr = drawTextureWithVerticesFromIndices_gl;
}

void renderFrame_gl(Renderer *renderer, void (*drawFunc)(Renderer *))
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	drawFunc(renderer);
	
	SDL_GL_SwapWindow(renderer->window);
	
#ifdef _DEBUG
	GLenum error;
	while((error = glGetError()) != GL_NO_ERROR)
	{
		fprintf(stderr, "Found OpenGL Error: %d\n", error);
	}
#endif
}

static GLuint glTextureFromPixelData(Renderer *renderer, const void *pixels, int32_t width, int32_t height, PixelFormat pixelFormat)
{
	GLuint texture = 0;
	
	GLenum format;
	switch (pixelFormat)
	{
		case PIXEL_FORMAT_BGRA32:
			format = GL_BGRA;
			break;
		case PIXEL_FORMAT_RGBA32:
			format = GL_RGBA;
			break;
	}
	
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D,
				 0,
				 GL_RGBA,
				 width, height,
				 0,
				 format,
				 GL_UNSIGNED_BYTE,
				 pixels);
	
	return texture;
}

TextureObject textureFromPixelData_gl(Renderer *renderer, const void *pixels, int32_t width, int32_t height, PixelFormat pixelFormat)
{
	GLuint texture = glTextureFromPixelData(renderer, pixels, width, height, pixelFormat);
	return (TextureObject){.glObject = texture};
}

void deleteTexture_gl(Renderer *renderer, TextureObject texture)
{
	glDeleteTextures(1, &texture.glObject);
}

static GLenum glModeFromMode(RendererMode mode)
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

BufferObject createIndexBufferObject_gl(Renderer *renderer, const void *data, uint32_t size)
{
	GLuint buffer = 0;
	glGenBuffers(1, &buffer);
	
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	return (BufferObject){.glObject = buffer};
}

BufferArrayObject createVertexArrayObject_gl(Renderer *renderer, const void *vertices, uint32_t verticesSize)
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
	glVertexAttribPointer(vertexAttributeIndex, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
	
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	return (BufferArrayObject){.glObject = vertexArray};
}

BufferArrayObject createVertexAndTextureCoordinateArrayObject_gl(Renderer *renderer, const void *verticesAndTextureCoordinates, uint32_t verticesSize, uint32_t textureCoordinatesSize)
{
	GLuint vertexArray = 0;
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);
	
	GLuint buffer = 0;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, verticesSize + textureCoordinatesSize, verticesAndTextureCoordinates, GL_STATIC_DRAW);
	
	glEnableVertexAttribArray(VERTEX_ATTRIBUTE);
	glVertexAttribPointer(VERTEX_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
	
	glEnableVertexAttribArray(TEXTURE_ATTRIBUTE);
	glVertexAttribPointer(TEXTURE_ATTRIBUTE, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid *)(uintptr_t)verticesSize);
	
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	return (BufferArrayObject){.glObject = vertexArray};
}

static void beginDrawingVertices(Shader_gl *shader, BufferArrayObject vertexArrayObject, RendererOptions options)
{
	bool disableDepthTest = (options & RENDERER_OPTION_DISABLE_DEPTH_TEST) != 0;
	if (disableDepthTest)
	{
		glDisable(GL_DEPTH_TEST);
	}
	
	bool blendingAlpha = (options & RENDERER_OPTION_BLENDING_ALPHA) != 0;
	bool blendingOneMinusAlpha = (options & RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA) != 0;
	bool blending = (blendingAlpha || blendingOneMinusAlpha);
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
	
	glBindVertexArray(vertexArrayObject.glObject);
	
	glUseProgram(shader->program);
}

static void setModelViewProjectionAndColorUniforms(Shader_gl *shader, float *modelViewProjectionMatrix, color4_t color)
{
	glUniformMatrix4fv(shader->modelViewProjectionMatrixUniformLocation, 1, GL_FALSE, modelViewProjectionMatrix);
	glUniform4f(shader->colorUniformLocation, color.red, color.green, color.blue, color.alpha);
}

static void endDrawingVerticesAndTextures(RendererOptions options)
{
	glBindVertexArray(0);
	
	bool blendingAlpha = (options & RENDERER_OPTION_BLENDING_ALPHA) != 0;
	bool blendingOneMinusAlpha = (options & RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA) != 0;
	bool blending = (blendingAlpha || blendingOneMinusAlpha);
	if (blending)
	{
		glDisable(GL_BLEND);
	}
	
	bool disableDepthTest = (options & RENDERER_OPTION_DISABLE_DEPTH_TEST) != 0;
	if (disableDepthTest)
	{
		glEnable(GL_DEPTH_TEST);
	}
}

void drawVertices_gl(Renderer *renderer, float *modelViewProjectionMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options)
{
	beginDrawingVertices(&renderer->glPositionShader, vertexArrayObject, options);
	
	setModelViewProjectionAndColorUniforms(&renderer->glPositionShader, modelViewProjectionMatrix, color);
	
	glDrawArrays(glModeFromMode(mode), 0, vertexCount);
	
	endDrawingVerticesAndTextures(options);
}

void drawVerticesFromIndices_gl(Renderer *renderer, float *modelViewProjectionMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options)
{
	beginDrawingVertices(&renderer->glPositionShader, vertexArrayObject, options);
	
	setModelViewProjectionAndColorUniforms(&renderer->glPositionShader, modelViewProjectionMatrix, color);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBufferObject.glObject);
	
	glDrawElements(glModeFromMode(mode), indicesCount, GL_UNSIGNED_SHORT, NULL);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	endDrawingVerticesAndTextures(options);
}

static void beginDrawingTexture(Shader_gl *shader, TextureObject texture, BufferArrayObject vertexAndTextureArrayObject, RendererOptions options)
{
	beginDrawingVertices(shader, vertexAndTextureArrayObject, options);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture.glObject);
	glUniform1i(shader->textureUniformLocation, 0);
}

void drawTextureWithVertices_gl(Renderer *renderer, float *modelViewProjectionMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options)
{
	beginDrawingTexture(&renderer->glPositionTextureShader, texture, vertexAndTextureArrayObject, options);
	
	setModelViewProjectionAndColorUniforms(&renderer->glPositionTextureShader, modelViewProjectionMatrix, color);
	
	glDrawArrays(glModeFromMode(mode), 0, vertexCount);
	
	endDrawingVerticesAndTextures(options);
}

void drawTextureWithVerticesFromIndices_gl(Renderer *renderer, float *modelViewProjectionMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options)
{
	beginDrawingTexture(&renderer->glPositionTextureShader, texture, vertexAndTextureArrayObject, options);
	
	setModelViewProjectionAndColorUniforms(&renderer->glPositionTextureShader, modelViewProjectionMatrix, color);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBufferObject.glObject);
	
	glDrawElements(glModeFromMode(mode), indicesCount, GL_UNSIGNED_SHORT, NULL);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	endDrawingVerticesAndTextures(options);
}
