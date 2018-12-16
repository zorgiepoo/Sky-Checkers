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
	int32_t program;
	
	int32_t modelViewProjectionMatrixUniformLocation;
	int32_t colorUniformLocation;
	int32_t textureUniformLocation;
} Shader_gl;

#define MAX_PIPELINE_COUNT 6

typedef struct _Renderer
{
	SDL_Window *window;
	float projectionMatrix[16];
	
	int32_t screenWidth;
	int32_t screenHeight;
	
	int32_t windowWidth;
	int32_t windowHeight;
	
	SDL_bool vsync;
	SDL_bool fsaa;
	
	enum
	{
		NDC_TYPE_GL,
		NDC_TYPE_METAL
	} ndcType;
	
	union
	{
		// Private GL data
		struct
		{
			Shader_gl glPositionTextureShader;
			Shader_gl glPositionShader;
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
	void (*deleteTexturePtr)(struct _Renderer *, TextureObject);
	BufferObject (*createBufferObjectPtr)(struct _Renderer *, const void *data, uint32_t size);
	BufferArrayObject (*createVertexArrayObjectPtr)(struct _Renderer *, const void *, uint32_t);
	BufferArrayObject (*createVertexAndTextureCoordinateArrayObjectPtr)(struct _Renderer *, const void *, uint32_t, uint32_t);
	void (*drawVerticesPtr)(struct _Renderer *, float *, RendererMode, BufferArrayObject, uint32_t, color4_t, RendererOptions);
	void (*drawVerticesFromIndicesPtr)(struct _Renderer *, float *, RendererMode, BufferArrayObject, BufferObject, uint32_t, color4_t, RendererOptions);
	void (*drawTextureWithVerticesPtr)(struct _Renderer *, float *, TextureObject, RendererMode, BufferArrayObject, uint32_t, color4_t, RendererOptions);
	void (*drawTextureWithVerticesFromIndicesPtr)(struct _Renderer *, float *, TextureObject, RendererMode, BufferArrayObject, BufferObject, uint32_t, color4_t, RendererOptions);
} Renderer;
