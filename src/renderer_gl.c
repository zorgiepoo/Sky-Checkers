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

#include "math_3d.h"
#include "utilities.h"

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

void renderFrame_gl(Renderer *renderer, void (*drawFunc)(Renderer *));

TextureObject textureFromPixelData_gl(Renderer *renderer, const void *pixels, int32_t width, int32_t height);

BufferObject createBufferObject_gl(Renderer *renderer, const void *data, uint32_t size);

BufferArrayObject createVertexArrayObject_gl(Renderer *renderer, const void *vertices, uint32_t verticesSize);

BufferArrayObject createVertexAndTextureCoordinateArrayObject_gl(Renderer *renderer, const void *verticesAndTextureCoordinates, uint32_t verticesSize, uint32_t textureCoordinatesSize);

void drawVertices_gl(Renderer *renderer, mat4_t modelViewMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options);

void drawVerticesFromIndices_gl(Renderer *renderer, mat4_t modelViewMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options);

void drawTextureWithVertices_gl(Renderer *renderer, mat4_t modelViewMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options);

void drawTextureWithVerticesFromIndices_gl(Renderer *renderer, mat4_t modelViewMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options);

void drawInstancedAlternatingTexturesWithVerticesFromIndices_gl(Renderer *renderer, mat4_t *modelViewProjectionMatrices, TextureObject texture1, TextureObject texture2, color4_t *colors, uint32_t *textureIndices, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, uint32_t instancesCount, RendererOptions options);

static SDL_bool compileShader(GLuint *shader, uint16_t glslVersion, GLenum type, const char *filepath)
{
	GLint status;
	
	FILE *sourceFile = fopen(filepath, "r");
	if (sourceFile == NULL)
	{
		zgPrint("Shader doesn't exist at: %s", filepath);
		return SDL_FALSE;
	}
	
	fseek(sourceFile, 0, SEEK_END);
	GLint fileSize = (GLint)ftell(sourceFile);
	fseek(sourceFile, 0, SEEK_SET);
	
	GLchar *source = (GLchar *)malloc(fileSize);
	fread(source, fileSize, 1, sourceFile);
	
	fclose(sourceFile);
	
	*shader = glCreateShader(type);
	
	GLchar versionLine[256] = {0};
	snprintf(versionLine, sizeof(versionLine) - 1, "#version %u\n", glslVersion);
	glShaderSource(*shader, 2, (const GLchar *[]){versionLine, source}, (GLint []){strlen(versionLine), fileSize});
	
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

static void compileAndLinkShader(Shader_gl *shader, uint16_t glslVersion, const char *vertexShaderPath, const char *fragmentShaderPath, SDL_bool textured, const char *modelViewProjectionUniform, const char *colorUniform, const char *textureSampleUniform, const char *textureIndicesUniform)
{
	// Create a pair of shaders
	GLuint vertexShader = 0;
	if (!compileShader(&vertexShader, glslVersion, GL_VERTEX_SHADER, vertexShaderPath))
	{
		zgPrint("Error: Failed to compile vertex shader: %s..\n", vertexShaderPath);
		exit(2);
	}
	
	GLuint fragmentShader = 0;
	if (!compileShader(&fragmentShader, glslVersion, GL_FRAGMENT_SHADER, fragmentShaderPath))
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
		zgPrint("Failed to link shader program");
		exit(2);
	}
	
	GLint modelViewProjectionMatrixUniformLocation = glGetUniformLocation(shaderProgram, modelViewProjectionUniform);
	if (modelViewProjectionMatrixUniformLocation == -1)
	{
		zgPrint("Failed to find %s uniform", modelViewProjectionUniform);
		exit(1);
	}
	shader->modelViewProjectionMatrixUniformLocation = modelViewProjectionMatrixUniformLocation;
	
	GLint colorUniformLocation = glGetUniformLocation(shaderProgram, colorUniform);
	if (colorUniformLocation == -1)
	{
		zgPrint("Failed to find %s uniform", colorUniform);
		exit(1);
	}
	shader->colorUniformLocation = colorUniformLocation;
	
	if (textured)
	{
		GLint textureUniformLocation = glGetUniformLocation(shaderProgram, textureSampleUniform);
		if (textureUniformLocation == -1)
		{
			zgPrint("Failed to find %s uniform", textureSampleUniform);
			exit(1);
		}
		shader->textureUniformLocation = textureUniformLocation;
	}
	
	if (textureIndicesUniform != NULL)
	{
		GLint textureIndicesUniformLocation = glGetUniformLocation(shaderProgram, textureIndicesUniform);
		if (textureIndicesUniformLocation == -1)
		{
			zgPrint("Failed to find %s uniform", textureIndicesUniform);
			exit(1);
		}
		shader->textureIndicesUniformLocation = textureIndicesUniformLocation;
	}
	
	shader->program = shaderProgram;
	
	glDetachShader(shaderProgram, vertexShader);
	glDeleteShader(vertexShader);
	
	glDetachShader(shaderProgram, fragmentShader);
	glDeleteShader(fragmentShader);
}

static SDL_bool createOpenGLContext(SDL_Window **window, SDL_GLContext *glContext, uint16_t glslVersion, const char *windowTitle, int32_t windowWidth, int32_t windowHeight, uint32_t videoFlags, SDL_bool fsaa)
{
	switch (glslVersion)
	{
		case 410:
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
			break;
		case 330:
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
			break;
		case 120:
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
			break;
		default:
			zgPrint("Invalid glsl version passed..");
			exit(1);
	}
	
	// Anti aliasing
	if (fsaa)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);
	}
	
	*window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, videoFlags | SDL_WINDOW_OPENGL);
	
	if (*window == NULL || (*glContext = SDL_GL_CreateContext(*window)) == NULL)
	{
		if (!fsaa)
		{
			return SDL_FALSE;
		}
		else
		{
			// Try creating SDL window and opengl context again except without anti-aliasing this time
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
			
			if (*window != NULL)
			{
				SDL_DestroyWindow(*window);
			}
			
			*window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, videoFlags | SDL_WINDOW_OPENGL);
			
			if (*window == NULL)
			{
				return SDL_FALSE;
			}
			
			*glContext = SDL_GL_CreateContext(*window);
			if (*glContext == NULL)
			{
				SDL_DestroyWindow(*window);
				
				return SDL_FALSE;
			}
		}
	}
	return SDL_TRUE;
}

void createRenderer_gl(Renderer *renderer, const char *windowTitle, int32_t windowWidth, int32_t windowHeight, uint32_t videoFlags, SDL_bool vsync, SDL_bool fsaa)
{
	// VSYNC
	SDL_GL_SetSwapInterval(vsync);
	
	// Buffer sizes
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	
	renderer->supportsInstancing = SDL_TRUE;
	
	uint16_t glslVersion = 410;
	SDL_GLContext glContext = NULL;
	if (!createOpenGLContext(&renderer->window, &glContext, glslVersion, windowTitle, windowWidth, windowHeight, videoFlags, fsaa))
	{
		glslVersion = 330;
		if (!createOpenGLContext(&renderer->window, &glContext, glslVersion, windowTitle, windowWidth, windowHeight, videoFlags, fsaa))
		{
			glslVersion = 120;
			if (!createOpenGLContext(&renderer->window, &glContext, glslVersion, windowTitle, windowWidth, windowHeight, videoFlags, fsaa))
			{
				zgPrint("Failed to create OpenGL context with even glsl version %i", glslVersion);
				exit(1);
			}
			else
			{
				renderer->supportsInstancing = SDL_FALSE;
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
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f);
	
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	
	compileAndLinkShader(&renderer->glPositionShader, glslVersion, "Data/Shaders/position.vsh", "Data/Shaders/position.fsh", SDL_FALSE, "modelViewProjectionMatrix", "color", NULL, NULL);
	
	compileAndLinkShader(&renderer->glPositionTextureShader, glslVersion, "Data/Shaders/texture-position.vsh", "Data/Shaders/texture-position.fsh", SDL_TRUE, "modelViewProjectionMatrix", "color", "textureSample", NULL);
	
	if (renderer->supportsInstancing)
	{
		compileAndLinkShader(&renderer->glTilesShader, glslVersion, "Data/Shaders/tiles.vsh", "Data/Shaders/tiles.fsh", SDL_TRUE, "modelViewProjectionMatrices", "colors", "textureSamples", "textureIndices");
	}
	
	renderer->renderFramePtr = renderFrame_gl;
	renderer->textureFromPixelDataPtr = textureFromPixelData_gl;
	renderer->createBufferObjectPtr = createBufferObject_gl;
	renderer->createVertexArrayObjectPtr = createVertexArrayObject_gl;
	renderer->createVertexAndTextureCoordinateArrayObjectPtr = createVertexAndTextureCoordinateArrayObject_gl;
	renderer->drawVerticesPtr = drawVertices_gl;
	renderer->drawVerticesFromIndicesPtr = drawVerticesFromIndices_gl;
	renderer->drawTextureWithVerticesPtr = drawTextureWithVertices_gl;
	renderer->drawTextureWithVerticesFromIndicesPtr = drawTextureWithVerticesFromIndices_gl;
	renderer->drawInstancedAlternatingTexturesWithVerticesFromIndicesPtr = drawInstancedAlternatingTexturesWithVerticesFromIndices_gl;
}

void renderFrame_gl(Renderer *renderer, void (*drawFunc)(Renderer *))
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	drawFunc(renderer);
	
	SDL_GL_SwapWindow(renderer->window);
}

TextureObject textureFromPixelData_gl(Renderer *renderer, const void *pixels, int32_t width, int32_t height)
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
	
	return (TextureObject){.glObject = tex};
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

BufferObject createBufferObject_gl(Renderer *renderer, const void *data, uint32_t size)
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

static void beginDrawingVertices(Shader_gl *shader, mat4_t modelViewMatrix, mat4_t projectionMatrix, BufferArrayObject vertexArrayObject, color4_t color, RendererOptions options)
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
	
	glBindVertexArray(vertexArrayObject.glObject);
	
	glUseProgram(shader->program);
	
	mat4_t modelViewProjectionMatrix = m4_mul(projectionMatrix, modelViewMatrix);
	glUniformMatrix4fv(shader->modelViewProjectionMatrixUniformLocation, 1, GL_FALSE, &modelViewProjectionMatrix.m00);
	glUniform4f(shader->colorUniformLocation, color.red, color.green, color.blue, color.alpha);
}

static void endDrawingVerticesAndTextures(RendererOptions options)
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

void drawVertices_gl(Renderer *renderer, mat4_t modelViewMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options)
{
	beginDrawingVertices(&renderer->glPositionShader, modelViewMatrix, renderer->projectionMatrix, vertexArrayObject, color, options);
	
	glDrawArrays(glModeFromMode(mode), 0, vertexCount);
	
	endDrawingVerticesAndTextures(options);
}

void drawVerticesFromIndices_gl(Renderer *renderer, mat4_t modelViewMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options)
{
	beginDrawingVertices(&renderer->glPositionShader, modelViewMatrix, renderer->projectionMatrix, vertexArrayObject, color, options);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBufferObject.glObject);
	
	glDrawElements(glModeFromMode(mode), indicesCount, GL_UNSIGNED_SHORT, NULL);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	endDrawingVerticesAndTextures(options);
}

static void beginDrawingTexture(Shader_gl *shader, mat4_t modelViewMatrix, mat4_t projectionMatrix, TextureObject texture, BufferArrayObject vertexAndTextureArrayObject, color4_t color, RendererOptions options)
{
	beginDrawingVertices(shader, modelViewMatrix, projectionMatrix, vertexAndTextureArrayObject, color, options);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture.glObject);
	glUniform1i(shader->textureUniformLocation, 0);
}

void drawTextureWithVertices_gl(Renderer *renderer, mat4_t modelViewMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options)
{
	beginDrawingTexture(&renderer->glPositionTextureShader, modelViewMatrix, renderer->projectionMatrix, texture, vertexAndTextureArrayObject, color, options);
	
	glDrawArrays(glModeFromMode(mode), 0, vertexCount);
	
	endDrawingVerticesAndTextures(options);
}

void drawTextureWithVerticesFromIndices_gl(Renderer *renderer, mat4_t modelViewMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options)
{
	beginDrawingTexture(&renderer->glPositionTextureShader, modelViewMatrix, renderer->projectionMatrix, texture, vertexAndTextureArrayObject, color, options);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBufferObject.glObject);
	
	glDrawElements(glModeFromMode(mode), indicesCount, GL_UNSIGNED_SHORT, NULL);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	endDrawingVerticesAndTextures(options);
}

void drawInstancedAlternatingTexturesWithVerticesFromIndices_gl(Renderer *renderer, mat4_t *modelViewProjectionMatrices, TextureObject texture1, TextureObject texture2, color4_t *colors, uint32_t *textureIndices, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, uint32_t instancesCount, RendererOptions options)
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
	
	glBindVertexArray(vertexAndTextureArrayObject.glObject);
	
	Shader_gl *shader = &renderer->glTilesShader;
	
	glUseProgram(shader->program);
	
	glUniformMatrix4fv(shader->modelViewProjectionMatrixUniformLocation, instancesCount, GL_FALSE, &modelViewProjectionMatrices->m00);
	glUniform4fv(shader->colorUniformLocation, instancesCount, &colors->red);
	glUniform1uiv(shader->textureIndicesUniformLocation, instancesCount, textureIndices);
	
	glUniform1i(shader->textureUniformLocation, 0);
	glUniform1i(shader->textureUniformLocation + 1, 1);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture1.glObject);
	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture2.glObject);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBufferObject.glObject);
	
	glDrawElementsInstanced(glModeFromMode(mode), indicesCount, GL_UNSIGNED_SHORT, NULL, instancesCount);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	glBindVertexArray(0);
	
	if (blending)
	{
		glDisable(GL_BLEND);
	}
	
	if (disableDepthTest)
	{
		glEnable(GL_DEPTH_TEST);
	}
}
