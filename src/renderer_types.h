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

#ifdef __cplusplus
extern "C" {
#endif

#include "platforms.h"
#include "float.h"
#include "window.h"
#include <stdbool.h>
#include <stdint.h>

#if !PLATFORM_IOS
#include "keyboard.h"
#endif

#define MSAA_PREFERRED_RETINA_SAMPLE_COUNT 2
#define MSAA_PREFERRED_NONRETINA_SAMPLE_COUNT 4

typedef struct
{
#if !PLATFORM_IOS
	void (*windowEventHandler)(ZGWindowEvent, void*);
	void* windowEventContext;
	void (*keyboardEventHandler)(ZGKeyboardEvent, void*);
	void* keyboardEventContext;
#endif

	const char* windowTitle;

	int32_t windowWidth;
	int32_t windowHeight;

	bool fullscreen;
	bool vsync;
	bool fsaa;
} RendererCreateOptions;

typedef enum
{
	RENDERER_OPTION_NONE = 0,
	RENDERER_OPTION_BLENDING_ALPHA = (1 << 0),
	RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA = (1 << 1)
} RendererOptions;

typedef enum
{
	RENDERER_TRIANGLE_MODE = 1,
	RENDERER_TRIANGLE_STRIP_MODE = 2
} RendererMode;

typedef struct
{
	ZGFloat red;
	ZGFloat green;
	ZGFloat blue;
	ZGFloat alpha;
} color4_t;

typedef struct
{
	union
	{
#if PLATFORM_APPLE
		void *metalObject;
#elif PLATFORM_WINDOWS
		void *d3d11Object;
#elif PLATFORM_LINUX
		uint32_t glObject;
#endif
	};
} BufferObject;

typedef struct
{
	union
	{
#if PLATFORM_APPLE
		struct
		{
			void *metalObject;
			uint32_t metalVerticesSize;
		};
#elif PLATFORM_WINDOWS
		struct
		{
			void *d3d11Object;
			uint32_t d3d11VerticesSize;
		};
#elif PLATFORM_LINUX
		uint32_t glObject;
#endif
	};
} BufferArrayObject;

typedef struct
{
	union
	{
#if PLATFORM_APPLE
		void *metalObject;
#elif PLATFORM_WINDOWS
		void *d3d11Object;
#elif PLATFORM_LINUX
		uint32_t glObject;
#endif
	};
} TextureObject;

typedef enum
{
	PIXEL_FORMAT_RGBA32,
	PIXEL_FORMAT_BGRA32
} PixelFormat;

#if PLATFORM_LINUX
typedef struct
{
	int32_t program;

	int32_t modelViewProjectionMatrixUniformLocation;
	int32_t colorUniformLocation;
	int32_t textureUniformLocation;
} Shader_gl;
#elif PLATFORM_WINDOWS
typedef struct
{
	void *vertexShader;
	void *pixelShader;
	void *vertexInputLayout;
} Shader_d3d11;
#endif

#define MAX_PIPELINE_COUNT 6

typedef struct _Renderer
{
	ZGWindow *window;
	ZGFloat projectionMatrix[16];

	// Width and height of drawable in pixels
	int32_t drawableWidth;
	int32_t drawableHeight;

	// Used for keeping track of width/height in points of window when not in fullscreen
	int32_t windowWidth;
	int32_t windowHeight;

	uint32_t sampleCount;

	bool fullscreen;
	bool vsync;
	bool fsaa;

	union
	{
#if PLATFORM_LINUX
		// Private GL data
		struct
		{
			Shader_gl glPositionTextureShader;
			Shader_gl glPositionShader;
		};
#elif PLATFORM_APPLE
		// Private metal data
		struct
		{
			void *metalLayer;
			void *metalCommandQueue;
			void *metalCurrentRenderCommandEncoder;
			void *metalRenderPassDescriptor;
			void *metalDepthTestStencilState;
			void *metalPipelineStates[MAX_PIPELINE_COUNT];
			void *metalShaderFunctions;
			bool metalWantsFsaa;
			bool metalCreatedInitialPipelines;
		};
#elif PLATFORM_WINDOWS
		// Private D3D11 data
		struct
		{
			void *d3d11Device;
			void *d3d11Context;
			void *d3d11RenderTargetView;
			void *d3d11DepthStencilView;
			void *d3d11DepthStencilBuffer;
			void *d3d11DepthStencilState;
			void *d3d11OneMinusAlphaBlendState;
			void *d3d11AlphaBlendState;
			void *d3d11SwapChain;
			Shader_d3d11 d3d11PositionShader;
			Shader_d3d11 d3d11TexturePositionShader;
			void *d3d11VertexShaderConstantBuffer;
			void *d3d11PixelShaderConstantBuffer;
			void *d3d11SamplerState;
		};
#endif
	};

	// Private function pointers
	// Note mat4_t is not passed in these function pointers or included in this file
	// because the matrix library is not very C++ friendly, and the d3d renderer is C++
	void(*updateViewportPtr)(struct _Renderer *, int32_t, int32_t);
	void(*renderFramePtr)(struct _Renderer *, void(*)(struct _Renderer *));
	TextureObject(*textureFromPixelDataPtr)(struct _Renderer *, const void *, int32_t, int32_t, PixelFormat);
	void(*deleteTexturePtr)(struct _Renderer *, TextureObject);
	BufferObject(*createIndexBufferObjectPtr)(struct _Renderer *, const void *data, uint32_t size);
	BufferArrayObject(*createVertexArrayObjectPtr)(struct _Renderer *, const void *, uint32_t);
	BufferArrayObject(*createVertexAndTextureCoordinateArrayObjectPtr)(struct _Renderer *, const void *, uint32_t, uint32_t);
	void(*drawVerticesPtr)(struct _Renderer *, ZGFloat *, RendererMode, BufferArrayObject, uint32_t, color4_t, RendererOptions);
	void(*drawVerticesFromIndicesPtr)(struct _Renderer *, ZGFloat *, RendererMode, BufferArrayObject, BufferObject, uint32_t, color4_t, RendererOptions);
	void(*drawTextureWithVerticesPtr)(struct _Renderer *, ZGFloat *, TextureObject, RendererMode, BufferArrayObject, uint32_t, color4_t, RendererOptions);
	void(*drawTextureWithVerticesFromIndicesPtr)(struct _Renderer *, ZGFloat *, TextureObject, RendererMode, BufferArrayObject, BufferObject, uint32_t, color4_t, RendererOptions);
	void(*pushDebugGroupPtr)(struct _Renderer *, const char *);
	void(*popDebugGroupPtr)(struct _Renderer *);
} Renderer;

#ifdef __cplusplus
}
#endif
