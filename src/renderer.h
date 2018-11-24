/*
 * Copyright 2010 Mayur Pawashe
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

#define RENDERER_OPTION_NONE 0
#define RENDERER_OPTION_DISABLE_DEPTH_TEST (1 << 0)
#define RENDERER_OPTION_BLENDING_ALPHA (1 << 1)
#define RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA (1 << 2)

#define RENDERER_INT8_TYPE 1
#define RENDERER_INT16_TYPE 2
#define RENDERER_FLOAT_TYPE 3

#define RENDERER_TRIANGLE_MODE 1
#define RENDERER_TRIANGLE_STRIP_MODE 2

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

void createRenderer(Renderer *renderer, int32_t windowWidth, int32_t windowHeight, uint32_t videoFlags, SDL_bool vsync, SDL_bool fsaa);

void clearColorAndDepthBuffers(Renderer *renderer);
void swapBuffers(Renderer *renderer);

void loadTexture(Renderer *renderer, const char *filePath, uint32_t *tex);
uint32_t textureFromPixelData(Renderer *renderer, const void *pixels, int32_t width, int32_t height);

uint32_t createVertexBufferObject(const void *data, uint32_t size);

uint32_t createVertexArrayObject(const void *vertices, uint32_t verticesSize, uint8_t vertexComponents);

uint32_t createVertexAndTextureCoordinateArrayObject(const void *verticesAndTextureCoordinates, uint32_t verticesSize, uint8_t vertexComponents, uint32_t textureCoordinatesSize, uint8_t textureCoordinateType);

void drawVertices(Renderer *renderer, mat4_t modelViewMatrix, uint8_t mode, uint32_t vertexArrayObject, uint32_t vertexCount, color4_t color, uint8_t options);

void drawVerticesFromIndices(Renderer *renderer, mat4_t modelViewMatrix, uint8_t mode, uint32_t vertexArrayObject, uint32_t indicesBufferObject, uint8_t indicesType, uint32_t indicesCount, color4_t color, uint8_t options);

void drawTextureWithVertices(Renderer *renderer, mat4_t modelViewMatrix, uint32_t texture, uint8_t mode, uint32_t vertexAndTextureArrayObject, uint32_t vertexCount, color4_t color, uint8_t options);

void drawTextureWithVerticesFromIndices(Renderer *renderer, mat4_t modelViewMatrix, uint32_t texture, uint8_t mode, uint32_t vertexAndTextureArrayObject, uint32_t indicesBufferObject, uint8_t indicesType, uint32_t indicesCount, color4_t color, uint8_t options);
