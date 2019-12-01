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
#include "platforms.h"
#include "window.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef MAC_OS_X
#include "renderer_metal.h"
#endif

#ifdef WINDOWS
#include "renderer_d3d11.h"
#endif

#ifdef linux
#include "renderer_gl.h"
#endif

void createRenderer(Renderer *renderer, int32_t windowWidth, int32_t windowHeight, bool fullscreen, bool vsync, bool fsaa)
{
#ifndef MAC_OS_X
	const char *windowTitle = "SkyCheckers";
#else
	const char *windowTitle = "";
#endif
	
	char *forceDisablingFSAAEnvironmentVariable = getenv("FORCE_DISABLE_AA");
	if (forceDisablingFSAAEnvironmentVariable != NULL && strlen(forceDisablingFSAAEnvironmentVariable) > 0 && (tolower(forceDisablingFSAAEnvironmentVariable[0]) == 'y' || forceDisablingFSAAEnvironmentVariable[0] == '1'))
	{
		fsaa = false;
		fprintf(stderr, "NOTICE: Force disabling anti-aliasing usage!!\n");
	}
	
#ifdef MAC_OS_X
	// Metal
	createRenderer_metal(renderer, windowTitle, windowWidth, windowHeight, fullscreen, vsync, fsaa);
#endif
	
#ifdef WINDOWS
	createRenderer_d3d11(renderer, windowTitle, windowWidth, windowHeight, fullscreen, vsync, fsaa);
#endif
	
#ifdef linux
	createRenderer_gl(renderer, windowTitle, windowWidth, windowHeight, fullscreen, vsync, fsaa);
#endif
}

void updateViewport(Renderer *renderer, int32_t windowWidth, int32_t windowHeight)
{	
	renderer->updateViewportPtr(renderer, windowWidth, windowHeight);
}

void renderFrame(Renderer *renderer, void (*drawFunc)(Renderer *))
{
	renderer->renderFramePtr(renderer, drawFunc);
}

TextureObject textureFromPixelData(Renderer *renderer, const void *pixels, int32_t width, int32_t height, PixelFormat pixelFormat)
{
	return renderer->textureFromPixelDataPtr(renderer, pixels, width, height, pixelFormat);
}

void deleteTexture(Renderer *renderer, TextureObject texture)
{
	renderer->deleteTexturePtr(renderer, texture);
}

BufferObject createIndexBufferObject(Renderer *renderer, const void *data, uint32_t size)
{
	return renderer->createIndexBufferObjectPtr(renderer, data, size);
}

BufferObject rectangleIndexBufferObject(Renderer *renderer)
{
	static BufferObject indicesBufferObject;
	static bool initializedBuffer = false;
	if (!initializedBuffer)
	{
		const uint16_t indices[] =
		{
			0, 1, 2,
			2, 3, 0
		};
		
		indicesBufferObject = createIndexBufferObject(renderer, indices, sizeof(indices));
		initializedBuffer = true;
	}
	return indicesBufferObject;
}

BufferArrayObject createVertexArrayObject(Renderer *renderer, const void *vertices, uint32_t verticesSize)
{
	return renderer->createVertexArrayObjectPtr(renderer, vertices, verticesSize);
}

BufferArrayObject createVertexAndTextureCoordinateArrayObject(Renderer *renderer, const void *verticesAndTextureCoordinates, uint32_t verticesSize, uint32_t textureCoordinatesSize)
{
	return renderer->createVertexAndTextureCoordinateArrayObjectPtr(renderer, verticesAndTextureCoordinates, verticesSize, textureCoordinatesSize);
}

static mat4_t computeModelViewProjectionMatrix(ZGFloat *projectionFloatMatrix, mat4_t modelViewMatrix)
{
	mat4_t projectionMatrix = *(mat4_t *)projectionFloatMatrix;
	return m4_mul(projectionMatrix, modelViewMatrix);
}

void drawVertices(Renderer *renderer, mat4_t modelViewMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options)
{
	mat4_t modelViewProjectionMatrix = computeModelViewProjectionMatrix(renderer->projectionMatrix, modelViewMatrix);
	renderer->drawVerticesPtr(renderer, &modelViewProjectionMatrix.m00, mode, vertexArrayObject, vertexCount, color, options);
}

void drawVerticesFromIndices(Renderer *renderer, mat4_t modelViewMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options)
{
	mat4_t modelViewProjectionMatrix = computeModelViewProjectionMatrix(renderer->projectionMatrix, modelViewMatrix);
	renderer->drawVerticesFromIndicesPtr(renderer, &modelViewProjectionMatrix.m00, mode, vertexArrayObject, indicesBufferObject, indicesCount, color, options);
}

void drawTextureWithVertices(Renderer *renderer, mat4_t modelViewMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options)
{
	mat4_t modelViewProjectionMatrix = computeModelViewProjectionMatrix(renderer->projectionMatrix, modelViewMatrix);
	renderer->drawTextureWithVerticesPtr(renderer, &modelViewProjectionMatrix.m00, texture, mode, vertexAndTextureArrayObject, vertexCount, color, options);
}

void drawTextureWithVerticesFromIndices(Renderer *renderer, mat4_t modelViewMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options)
{
	mat4_t modelViewProjectionMatrix = computeModelViewProjectionMatrix(renderer->projectionMatrix, modelViewMatrix);
	renderer->drawTextureWithVerticesFromIndicesPtr(renderer, &modelViewProjectionMatrix.m00, texture, mode, vertexAndTextureArrayObject, indicesBufferObject, indicesCount, color, options);
}
