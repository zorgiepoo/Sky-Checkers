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
#endif

void createRenderer(Renderer *renderer, int32_t windowWidth, int32_t windowHeight, uint32_t videoFlags, SDL_bool vsync, SDL_bool fsaa)
{
	// Metal
#ifdef MAC_OS_X
	createRenderer_metal(renderer, windowWidth, windowHeight, videoFlags, vsync, fsaa);

	renderer->renderFramePtr = renderFrame_metal;
	renderer->textureFromPixelDataPtr = textureFromPixelData_metal;
	renderer->createBufferObjectPtr = createBufferObject_metal;
	renderer->createVertexArrayObjectPtr = createVertexArrayObject_metal;
	renderer->createVertexAndTextureCoordinateArrayObjectPtr = createVertexAndTextureCoordinateArrayObject_metal;
	renderer->drawVerticesPtr = drawVertices_metal;
	renderer->drawVerticesFromIndicesPtr = drawVerticesFromIndices_metal;
	renderer->drawTextureWithVerticesPtr = drawTextureWithVertices_metal;
	renderer->drawTextureWithVerticesFromIndicesPtr = drawTextureWithVerticesFromIndices_metal;
#endif

	// OpenGL
//	createRenderer_gl(renderer, windowWidth, windowHeight, videoFlags, vsync, fsaa);
//
//	renderer->renderFramePtr = renderFrame_gl;
//	renderer->textureFromPixelDataPtr = textureFromPixelData_gl;
//	renderer->createBufferObjectPtr = createBufferObject_gl;
//	renderer->createVertexArrayObjectPtr = createVertexArrayObject_gl;
//	renderer->createVertexAndTextureCoordinateArrayObjectPtr = createVertexAndTextureCoordinateArrayObject_gl;
//	renderer->drawVerticesPtr = drawVertices_gl;
//	renderer->drawVerticesFromIndicesPtr = drawVerticesFromIndices_gl;
//	renderer->drawTextureWithVerticesPtr = drawTextureWithVertices_gl;
//	renderer->drawTextureWithVerticesFromIndicesPtr = drawTextureWithVerticesFromIndices_gl;
	
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
