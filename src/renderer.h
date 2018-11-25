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

#pragma once

#include "maincore.h"
#include "math_3d.h"

typedef enum
{
	RENDERER_OPTION_NONE = 0,
	RENDERER_OPTION_DISABLE_DEPTH_TEST = (1 << 0),
	RENDERER_OPTION_BLENDING_ALPHA = (1 << 1),
	RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA = (1 << 2)
} RendererOptions;

typedef enum
{
	RENDERER_TRIANGLE_MODE = 1,
	RENDERER_TRIANGLE_STRIP_MODE = 2
} RendererMode;

typedef enum
{
	RENDERER_INT8_TYPE = 1,
	RENDERER_INT16_TYPE = 2,
	RENDERER_FLOAT_TYPE = 3
} RendererType;

typedef struct
{
	int32_t program;
	int32_t modelViewProjectionMatrixUniformLocation;
	int32_t colorUniformLocation;
	int32_t textureUniformLocation;
} Shader;

typedef struct
{
	SDL_Window *window;
	mat4_t projectionMatrix;
	
	int32_t screenWidth;
	int32_t screenHeight;
	
	int32_t windowWidth;
	int32_t windowHeight;
	
	SDL_bool vsync;
	SDL_bool fsaa;
	
	Shader positionTextureShader;
	Shader positionShader;
} Renderer;

typedef struct
{
	float red;
	float green;
	float blue;
	float alpha;
} color4_t;

typedef struct
{
	uint32_t glObject;
} BufferArrayObject;

typedef struct
{
	uint32_t glObject;
} BufferObject;

typedef struct
{
	uint32_t glObject;
} TextureObject;

void createRenderer(Renderer *renderer, int32_t windowWidth, int32_t windowHeight, uint32_t videoFlags, SDL_bool vsync, SDL_bool fsaa);

void clearColorAndDepthBuffers(Renderer *renderer);
void swapBuffers(Renderer *renderer);

TextureObject loadTexture(Renderer *renderer, const char *filePath);
TextureObject textureFromPixelData(Renderer *renderer, const void *pixels, int32_t width, int32_t height);

BufferObject createBufferObject(const void *data, uint32_t size);

BufferArrayObject createVertexArrayObject(const void *vertices, uint32_t verticesSize, uint8_t vertexComponents);

BufferArrayObject createVertexAndTextureCoordinateArrayObject(const void *verticesAndTextureCoordinates, uint32_t verticesSize, uint8_t vertexComponents, uint32_t textureCoordinatesSize, RendererType textureCoordinateType);

void drawVertices(Renderer *renderer, mat4_t modelViewMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options);

void drawVerticesFromIndices(Renderer *renderer, mat4_t modelViewMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, BufferObject indicesBufferObject, RendererType indicesType, uint32_t indicesCount, color4_t color, RendererOptions options);

void drawTextureWithVertices(Renderer *renderer, mat4_t modelViewMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options);

void drawTextureWithVerticesFromIndices(Renderer *renderer, mat4_t modelViewMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, BufferObject indicesBufferObject, RendererType indicesType, uint32_t indicesCount, color4_t color, RendererOptions options);
