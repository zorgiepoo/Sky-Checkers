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

#include "math_3d.h"

#define MSAA_SAMPLE_COUNT 4

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

typedef struct
{
	float red;
	float green;
	float blue;
	float alpha;
} color4_t;

typedef struct
{
	union
	{
		void *metalObject;
		uint32_t glObject;
	};
} BufferObject;

typedef struct
{
	union
	{
		struct
		{
			void *metalObject;
			uint32_t metalVerticesSize;
		};
		uint32_t glObject;
	};
} BufferArrayObject;

typedef struct
{
	union
	{
		void *metalObject;
		uint32_t glObject;
	};
} TextureObject;

typedef struct
{
	union
	{
		void *metalObject;
		uint32_t glObject;
	};
} TextureArrayObject;

typedef struct
{
	int32_t program;
	
	int32_t modelViewProjectionMatrixUniformLocation;
	int32_t colorUniformLocation;
	int32_t textureUniformLocation;
	// For tile instancing
	int32_t textureIndicesUniformLocation;
} Shader_gl;

#define MAX_PIPELINE_COUNT 12

typedef struct _Renderer
{
	SDL_Window *window;
	mat4_t projectionMatrix;
	
	int32_t screenWidth;
	int32_t screenHeight;
	
	int32_t windowWidth;
	int32_t windowHeight;
	
	SDL_bool vsync;
	SDL_bool fsaa;
	
	SDL_bool supportsInstancing;
	
	union
	{
		// Private GL data
		struct
		{
			Shader_gl glPositionTextureShader;
			Shader_gl glPositionShader;
			Shader_gl glInstancedTexturesShader;
			Shader_gl glInstancedTextureShader;
		};
		
		// Private metal data
		struct
		{
			void *metalLayer;
			void *metalCommandQueue;
			void *metalCurrentRenderCommandEncoder;
			void *metalDepthTexture;
			void *metalMultisampleTexture;
			void *metalDepthTestStencilState;
			void *metalPipelineStates[MAX_PIPELINE_COUNT];
		};
	};
	
	// Private function pointers
	void (*updateViewportPtr)(struct _Renderer *);
	void (*renderFramePtr)(struct _Renderer *, void (*)(struct _Renderer *));
	TextureObject (*textureFromPixelDataPtr)(struct _Renderer *, const void *, int32_t, int32_t);
	TextureArrayObject (*textureArrayFromPixelDataPtr)(struct _Renderer *, const void *, int32_t, int32_t);
	BufferObject (*createBufferObjectPtr)(struct _Renderer *, const void *data, uint32_t size);
	BufferArrayObject (*createVertexArrayObjectPtr)(struct _Renderer *, const void *, uint32_t);
	BufferArrayObject (*createVertexAndTextureCoordinateArrayObjectPtr)(struct _Renderer *, const void *, uint32_t, uint32_t);
	void (*drawVerticesPtr)(struct _Renderer *, mat4_t, RendererMode, BufferArrayObject, uint32_t, color4_t, RendererOptions);
	void (*drawVerticesFromIndicesPtr)(struct _Renderer *, mat4_t, RendererMode, BufferArrayObject, BufferObject, uint32_t, color4_t, RendererOptions);
	void (*drawTextureWithVerticesPtr)(struct _Renderer *, mat4_t, TextureObject, RendererMode, BufferArrayObject, uint32_t, color4_t, RendererOptions);
	void (*drawTextureWithVerticesFromIndicesPtr)(struct _Renderer *, mat4_t, TextureObject, RendererMode, BufferArrayObject, BufferObject, uint32_t, color4_t, RendererOptions);
	void (*drawInstancedTextureArrayWithVerticesFromIndicesPtr)(struct _Renderer *, mat4_t *, TextureArrayObject, color4_t *, uint32_t *, RendererMode, BufferArrayObject, BufferObject, uint32_t, uint32_t, RendererOptions);
	void (*drawInstancedTextureWithVerticesFromIndicesPtr)(struct _Renderer *, mat4_t *, TextureObject, color4_t *, RendererMode, BufferArrayObject, BufferObject, uint32_t, uint32_t, RendererOptions);
} Renderer;
