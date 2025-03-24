/*
 MIT License

 Copyright (c) 2024 Mayur Pawashe

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

#include "renderer.h"
#include "platforms.h"
#include "window.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if PLATFORM_APPLE
#include "renderer_metal.h"
#elif PLATFORM_WINDOWS
#include "renderer_d3d11.h"
#elif PLATFORM_LINUX
#include "renderer_gl.h"
#endif

void createRenderer(Renderer *renderer, RendererCreateOptions options)
{
	char *forceDisablingFSAAEnvironmentVariable = getenv("FORCE_DISABLE_AA");
	if (forceDisablingFSAAEnvironmentVariable != NULL && strlen(forceDisablingFSAAEnvironmentVariable) > 0 && (tolower(forceDisablingFSAAEnvironmentVariable[0]) == 'y' || forceDisablingFSAAEnvironmentVariable[0] == '1'))
	{
		options.fsaa = false;
		fprintf(stderr, "NOTICE: Force disabling anti-aliasing usage!!\n");
	}
	
#if PLATFORM_APPLE
	if (!createRenderer_metal(renderer, options))
	{
		fprintf(stderr, "Failed to create Metal renderer\n");
		abort();
	}
#elif PLATFORM_WINDOWS
	if (!createRenderer_d3d11(renderer, options))
	{
		fprintf(stderr, "Failed to create D3D renderer\n");
		abort();
	}
#elif PLATFORM_LINUX
	createRenderer_gl(renderer, options);
#endif
}

void updateViewport(Renderer *renderer, int32_t windowWidth, int32_t windowHeight)
{	
	renderer->updateViewportPtr(renderer, windowWidth, windowHeight);
}

void renderFrame(Renderer *renderer, void (*drawFunc)(Renderer *, void *), void *context)
{
	renderer->renderFramePtr(renderer, drawFunc, context);
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

void pushDebugGroup(Renderer *renderer, const char *debugGroupName)
{
	renderer->pushDebugGroupPtr(renderer, debugGroupName);
}

void popDebugGroup(Renderer *renderer)
{
	renderer->popDebugGroupPtr(renderer);
}
