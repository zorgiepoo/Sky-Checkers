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
#include "renderer_types.h"
#include "math_3d.h"

void createRenderer_metal(Renderer *renderer, int32_t windowWidth, int32_t windowHeight, uint32_t videoFlags, SDL_bool vsync, SDL_bool fsaa);

void renderFrame_metal(Renderer *renderer, void (*drawFunc)(Renderer *));

TextureObject textureFromPixelData_metal(Renderer *renderer, const void *pixels, int32_t width, int32_t height);

BufferObject createBufferObject_metal(Renderer *renderer, const void *data, uint32_t size);

BufferArrayObject createVertexArrayObject_metal(Renderer *renderer, const void *vertices, uint32_t verticesSize);

BufferArrayObject createVertexAndTextureCoordinateArrayObject_metal(Renderer *renderer, const void *verticesAndTextureCoordinates, uint32_t verticesSize, uint32_t textureCoordinatesSize);

void drawVertices_metal(Renderer *renderer, mat4_t modelViewMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options);

void drawVerticesFromIndices_metal(Renderer *renderer, mat4_t modelViewMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options);

void drawTextureWithVertices_metal(Renderer *renderer, mat4_t modelViewMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options);

void drawTextureWithVerticesFromIndices_metal(Renderer *renderer, mat4_t modelViewMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options);