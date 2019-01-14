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

#define MSAA_PREFERRED_RETINA_SAMPLE_COUNT 2
#define MSAA_PREFERRED_NONRETINA_SAMPLE_COUNT 4

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
#ifdef MAC_OS_X
		void *metalObject;
#endif
		uint32_t glObject;
	};
} BufferObject;

typedef struct
{
	union
	{
#ifdef MAC_OS_X
		struct
		{
			void *metalObject;
			uint32_t metalVerticesSize;
		};
#endif
		uint32_t glObject;
	};
} BufferArrayObject;

typedef struct
{
	union
	{
#ifdef MAC_OS_X
		void *metalObject;
#endif
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
	
	// Width and height of drawable in pixels
	int32_t drawableWidth;
	int32_t drawableHeight;
	
	// Used for keeping track of width/height in points of window when not in fullscreen
	int32_t windowWidth;
	int32_t windowHeight;
	
	uint32_t sampleCount;
	
	SDL_bool fullscreen;
	SDL_bool vsync;
	SDL_bool fsaa;
	
	enum
	{
		NDC_TYPE_GL
#ifdef MAC_OS_X
		,NDC_TYPE_METAL
#endif
	} ndcType;
	
	union
	{
		// Private GL data
		struct
		{
			Shader_gl glPositionTextureShader;
			Shader_gl glPositionShader;
		};
		
#ifdef MAC_OS_X
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
			void *metalShaderFunctions;
			SDL_bool metalWantsFsaa;
			SDL_bool metalCreatedInitialPipelines;
			SDL_bool metalIgnoreFirstFullscreenTransition;
		};
#endif
	};
	
#ifdef MAC_OS_X
	SDL_bool macosInFullscreenTransition;
#endif
	
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
