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

static const SDL_bool FORCE_OPENGL = SDL_FALSE;
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
	if (!FORCE_OPENGL)
	{
		// Metal
		createdRenderer = createRenderer_metal(renderer, windowTitle, windowWidth, windowHeight, videoFlags, vsync, fsaa);
#ifdef _DEBUG
		if (!createdRenderer)
		{
			zgPrint("ERROR: Failed creating Metal renderer!! Falling back to OpenGL.");
		}
#endif
	}
	else
	{
		zgPrint("NOTICE: Forcing OpenGL usage!!");
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

void renderFrame(Renderer *renderer, void (*drawFunc)(Renderer *))
{
	renderer->renderFramePtr(renderer, drawFunc);
}

TextureObject textureFromPixelData(Renderer *renderer, const void *pixels, int32_t width, int32_t height)
{
	return renderer->textureFromPixelDataPtr(renderer, pixels, width, height);
}

TextureArrayObject textureArrayFromPixelData(Renderer *renderer, const void *pixels, int32_t width, int32_t height)
{
	return renderer->textureArrayFromPixelDataPtr(renderer, pixels, width, height);
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

void drawInstancedTexturesWithVerticesFromIndices(Renderer *renderer, mat4_t *modelViewProjectionMatrices, TextureArrayObject textureArray, color4_t *colors, uint32_t *textureArrayIndices, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, uint32_t instancesCount, RendererOptions options)
{
	renderer->drawInstancedTexturesWithVerticesFromIndicesPtr(renderer, modelViewProjectionMatrices, textureArray, colors, textureArrayIndices, mode, vertexAndTextureArrayObject, indicesBufferObject, indicesCount, instancesCount, options);
}

void drawInstancedTextureWithVerticesFromIndices(Renderer *renderer, mat4_t *modelViewProjectionMatrices, TextureObject texture, color4_t *colors, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, uint32_t instancesCount, RendererOptions options)
{
	renderer->drawInstancedTextureWithVerticesFromIndicesPtr(renderer, modelViewProjectionMatrices, texture, colors, mode, vertexAndTextureArrayObject, indicesBufferObject, indicesCount, instancesCount, options);
}
