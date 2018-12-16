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

#include "renderer.h"
#include "renderer_gl.h"

#ifdef MAC_OS_X
#include "renderer_metal.h"
#include "utilities.h"
#endif

void createRenderer(Renderer *renderer, int32_t windowWidth, int32_t windowHeight, uint32_t videoFlags, SDL_bool vsync, SDL_bool fsaa)
{
#ifndef MAC_OS_X
	const char *windowTitle = "SkyCheckers";
#else
	const char *windowTitle = "";
#endif
	
	SDL_bool createdRenderer = SDL_FALSE;
	
#ifdef MAC_OS_X
	SDL_bool forcingOpenGL = SDL_FALSE;
	char *forceOpenGLEnvironmentVariable = getenv("FORCE_OPENGL");
	if (forceOpenGLEnvironmentVariable != NULL && strlen(forceOpenGLEnvironmentVariable) > 0 && (tolower(forceOpenGLEnvironmentVariable[0]) == 'y' || forceOpenGLEnvironmentVariable[0] == '1'))
	{
		forcingOpenGL = SDL_TRUE;
	}
#endif
	
#ifdef MAC_OS_X
	if (!forcingOpenGL)
	{
		// Metal
		createdRenderer = createRenderer_metal(renderer, windowTitle, windowWidth, windowHeight, videoFlags, vsync, fsaa);
#ifdef _DEBUG
		if (!createdRenderer)
		{
			fprintf(stderr, "ERROR: Failed creating Metal renderer!! Falling back to OpenGL.\n");
		}
#endif
	}
	else
	{
		fprintf(stderr, "NOTICE: Forcing OpenGL usage!!\n");
	}
#endif

	// OpenGL
	if (!createdRenderer)
	{
		createRenderer_gl(renderer, windowTitle, windowWidth, windowHeight, videoFlags, vsync, fsaa);
		createdRenderer = SDL_TRUE;
	}
	
#ifndef _DEBUG
	SDL_ShowCursor(SDL_DISABLE);
#endif
}

void updateViewport(Renderer *renderer, int32_t windowWidth, int32_t windowHeight)
{
	renderer->windowWidth = windowWidth;
	renderer->windowHeight = windowHeight;
	
	renderer->updateViewportPtr(renderer);
}

void renderFrame(Renderer *renderer, void (*drawFunc)(Renderer *))
{
	renderer->renderFramePtr(renderer, drawFunc);
}

TextureObject textureFromPixelData(Renderer *renderer, const void *pixels, int32_t width, int32_t height)
{
	return renderer->textureFromPixelDataPtr(renderer, pixels, width, height);
}

void deleteTexture(Renderer *renderer, TextureObject texture)
{
	return renderer->deleteTexturePtr(renderer, texture);
}

BufferObject createBufferObject(Renderer *renderer, const void *data, uint32_t size)
{
	return renderer->createBufferObjectPtr(renderer, data, size);
}

BufferArrayObject createVertexArrayObject(Renderer *renderer, const void *vertices, uint32_t verticesSize)
{
	return renderer->createVertexArrayObjectPtr(renderer, vertices, verticesSize);
}

BufferArrayObject createVertexAndTextureCoordinateArrayObject(Renderer *renderer, const void *verticesAndTextureCoordinates, uint32_t verticesSize, uint32_t textureCoordinatesSize)
{
	return renderer->createVertexAndTextureCoordinateArrayObjectPtr(renderer, verticesAndTextureCoordinates, verticesSize, textureCoordinatesSize);
}

void drawVertices(Renderer *renderer, mat4_t modelViewMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options)
{
	renderer->drawVerticesPtr(renderer, modelViewMatrix, mode, vertexArrayObject, vertexCount, color, options);
}

void drawVerticesFromIndices(Renderer *renderer, mat4_t modelViewMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options)
{
	renderer->drawVerticesFromIndicesPtr(renderer, modelViewMatrix, mode, vertexArrayObject, indicesBufferObject, indicesCount, color, options);
}

void drawTextureWithVertices(Renderer *renderer, mat4_t modelViewMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options)
{
	renderer->drawTextureWithVerticesPtr(renderer, modelViewMatrix, texture, mode, vertexAndTextureArrayObject, vertexCount, color, options);
}

void drawTextureWithVerticesFromIndices(Renderer *renderer, mat4_t modelViewMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options)
{
	renderer->drawTextureWithVerticesFromIndicesPtr(renderer, modelViewMatrix, texture, mode, vertexAndTextureArrayObject, indicesBufferObject, indicesCount, color, options);
}
