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

#import "renderer_metal.h"
#include "utilities.h"
#include "metal_indices.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#define DEPTH_STENCIL_PIXEL_FORMAT MTLPixelFormatDepth16Unorm
#define MSAA_SAMPLE_COUNT 4

void createRenderer_metal(Renderer *renderer, int32_t windowWidth, int32_t windowHeight, uint32_t videoFlags, SDL_bool vsync, SDL_bool fsaa)
{
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
	
#ifndef MAC_OS_X
	const char *windowTitle = "SkyCheckers";
#else
	const char *windowTitle = "";
#endif
	renderer->window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, videoFlags);
	
	if (renderer->window == NULL)
	{
		zgPrint("Failed to create Metal renderer window! %e");
		SDL_Quit();
	}
	
	SDL_RendererFlags sdlRenderFlags = vsync ? SDL_RENDERER_PRESENTVSYNC : 0;
	SDL_Renderer *sdlRenderer = SDL_CreateRenderer(renderer->window, -1, sdlRenderFlags);
	if (sdlRenderer == NULL)
	{
		zgPrint("Failed to create SDL renderer for Metal! %e");
		SDL_Quit();
	}
	
	CAMetalLayer *metalLayer = (__bridge CAMetalLayer *)(SDL_RenderGetMetalLayer(sdlRenderer));
	
	SDL_DestroyRenderer(sdlRenderer);
	
	if (@available(macOS 10.13, *))
	{
		renderer->vsync = metalLayer.displaySyncEnabled;
	}
	else
	{
		renderer->vsync = vsync;
	}
	
	CGSize drawableSize = metalLayer.drawableSize;
	renderer->screenWidth = (int32_t)drawableSize.width;
	renderer->screenHeight = (int32_t)drawableSize.height;
	
	// https://metashapes.com/blog/opengl-metal-projection-matrix-problem/
	mat4_t metalProjectionAdjustMatrix =
	mat4(
		 1.0f, 0.0f, 0.0f, 0.0f,
		 0.0f, 1.0f, 0.0f, 0.0f,
		 0.0f, 0.0f, 0.5f, 0.5f,
		 0.0f, 0.0f, 0.0f, 1.0f
	);
	
	// The aspect ratio is not quite correct, which is a mistake I made a long time ago that is too troubling to fix properly
	renderer->projectionMatrix = m4_mul(metalProjectionAdjustMatrix, m4_perspective(45.0f, (float)(renderer->screenWidth / renderer->screenHeight), 10.0f, 300.0f));
	
	SDL_GetWindowSize(renderer->window, &renderer->windowWidth, &renderer->windowHeight);
	
	id<MTLDevice> device = metalLayer.device;
	
	renderer->fsaa = (fsaa && [device supportsTextureSampleCount:MSAA_SAMPLE_COUNT]);
	
	if (renderer->fsaa)
	{
		MTLTextureDescriptor *multisampleTextureDescriptor = [MTLTextureDescriptor new];
		multisampleTextureDescriptor.pixelFormat = metalLayer.pixelFormat;
		multisampleTextureDescriptor.width = (NSUInteger)renderer->screenWidth;
		multisampleTextureDescriptor.height = (NSUInteger)renderer->screenHeight;
		multisampleTextureDescriptor.resourceOptions = MTLResourceStorageModePrivate;
		multisampleTextureDescriptor.usage = MTLTextureUsageRenderTarget;
		multisampleTextureDescriptor.sampleCount = MSAA_SAMPLE_COUNT;
		multisampleTextureDescriptor.textureType = MTLTextureType2DMultisample;
		
		id<MTLTexture> multisampleTexture = [device newTextureWithDescriptor:multisampleTextureDescriptor];
		renderer->metalMultisampleTexture = (void *)CFBridgingRetain(multisampleTexture);
	}
	else
	{
		renderer->metalMultisampleTexture = NULL;
	}
	
	id<MTLLibrary> defaultLibrary = [device newDefaultLibrary];
	if (defaultLibrary == nil)
	{
		zgPrint("Failed to find default metal library");
		SDL_Quit();
	}
	
	id<MTLFunction> texturePositionVertexShader = [defaultLibrary newFunctionWithName:@"texturePositionVertexShader"];
	if (texturePositionVertexShader == nil)
	{
		zgPrint("Failed to find texture position vertex shader");
		SDL_Quit();
	}
	
	id<MTLFunction> texturePositionFragmentShader = [defaultLibrary newFunctionWithName:@"texturePositionFragmentShader"];
	if (texturePositionFragmentShader == nil)
	{
		zgPrint("Failed to find texture position fragment shader");
		SDL_Quit();
	}
	
	MTLRenderPipelineDescriptor *pipelineStateDescriptor = [MTLRenderPipelineDescriptor new];
	pipelineStateDescriptor.vertexFunction = texturePositionVertexShader;
	pipelineStateDescriptor.fragmentFunction = texturePositionFragmentShader;
	pipelineStateDescriptor.colorAttachments[0].pixelFormat = metalLayer.pixelFormat;
	pipelineStateDescriptor.colorAttachments[0].blendingEnabled = YES;
	pipelineStateDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
	pipelineStateDescriptor.depthAttachmentPixelFormat = DEPTH_STENCIL_PIXEL_FORMAT;
	if (renderer->fsaa)
	{
		pipelineStateDescriptor.sampleCount = MSAA_SAMPLE_COUNT;
	}
	
	NSError *pipelineError = nil;
	id<MTLRenderPipelineState> pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&pipelineError];
	
	if (pipelineState == nil)
	{
		NSLog(@"Pipeline state error: %@", pipelineError);
		SDL_Quit();
	}
	
	MTLDepthStencilDescriptor *depthStencilDescriptor = [MTLDepthStencilDescriptor new];
	depthStencilDescriptor.depthCompareFunction = MTLCompareFunctionLessEqual;
	depthStencilDescriptor.depthWriteEnabled = YES;
	
	id<MTLDepthStencilState> depthStencilState = [device newDepthStencilStateWithDescriptor:depthStencilDescriptor];
	if (depthStencilState == nil)
	{
		zgPrint("Depth stencil state failed to be created");
		SDL_Quit();
	}
	
	MTLTextureDescriptor *depthTextureDescriptor = [MTLTextureDescriptor new];
	depthTextureDescriptor.pixelFormat = DEPTH_STENCIL_PIXEL_FORMAT;
	depthTextureDescriptor.width = (NSUInteger)renderer->screenWidth;
	depthTextureDescriptor.height = (NSUInteger)renderer->screenHeight;
	depthTextureDescriptor.resourceOptions = MTLResourceStorageModePrivate;
	depthTextureDescriptor.usage = MTLTextureUsageRenderTarget;
	if (renderer->fsaa)
	{
		depthTextureDescriptor.sampleCount = MSAA_SAMPLE_COUNT;
		depthTextureDescriptor.textureType = MTLTextureType2DMultisample;
	}
	
	id<MTLTexture> depthTexture = [device newTextureWithDescriptor:depthTextureDescriptor];
	
	renderer->metalDepthTexture = (void *)CFBridgingRetain(depthTexture);
	renderer->metalBlendingSrcAlphaTexturePositionPipelineState = (void *)CFBridgingRetain(pipelineState);
	renderer->metalDepthTestEnabledPipelineState = (void *)CFBridgingRetain(depthStencilState);
	
//	id<MTLFunction> positionVertexShader = [defaultLibrary newFunctionWithName:@"positionVertexShader"];
//	if (positionVertexShader == nil)
//	{
//		zgPrint("Failed to find position vertex shader");
//		SDL_Quit();
//	}
//
//	id<MTLFunction> positionFragmentShader = [defaultLibrary newFunctionWithName:@"positionFragmentShader"];
//	if (positionFragmentShader == nil)
//	{
//		zgPrint("Failed to find position fragment shader");
//		SDL_Quit();
//	}
//
//	MTLRenderPipelineDescriptor *pipelineStateDescriptor2 = [MTLRenderPipelineDescriptor new];
//	pipelineStateDescriptor2.vertexFunction = positionVertexShader;
//	pipelineStateDescriptor2.fragmentFunction = positionFragmentShader;
//	pipelineStateDescriptor2.colorAttachments[0].pixelFormat = metalLayer.pixelFormat;
//	pipelineStateDescriptor2.depthAttachmentPixelFormat = DEPTH_STENCIL_PIXEL_FORMAT;
//
//	NSError *pipelineError2 = nil;
//	id<MTLRenderPipelineState> pipelineState2 = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor2 error:&pipelineError2];
//
//	if (pipelineState2 == nil)
//	{
//		NSLog(@"Pipeline state error: %@", pipelineError2);
//		SDL_Quit();
//	}
//
//	renderer->metalPositionPipelineState = (void *)CFBridgingRetain(pipelineState2);
	
	id<MTLCommandQueue> queue = [device newCommandQueue];
	
	renderer->metalLayer = (void *)CFBridgingRetain(metalLayer);
	renderer->metalCommandQueue = (void *)CFBridgingRetain(queue);
	renderer->metalCurrentRenderCommandEncoder = NULL;
}

void renderFrame_metal(Renderer *renderer, void (*drawFunc)(Renderer *))
{
	@autoreleasepool
	{
		CAMetalLayer *metalLayer = (__bridge CAMetalLayer *)(renderer->metalLayer);
		id<MTLCommandQueue> queue = (__bridge id<MTLCommandQueue>)(renderer->metalCommandQueue);
		
		id<CAMetalDrawable> drawable = [metalLayer nextDrawable];
		
		if (drawable != nil)
		{
			MTLClearColor clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
			
			MTLRenderPassDescriptor *passDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
			
			MTLRenderPassColorAttachmentDescriptor *primaryColorAttachment = passDescriptor.colorAttachments[0];
			
			primaryColorAttachment.clearColor = clearColor;
			primaryColorAttachment.loadAction  = MTLLoadActionClear;
			
			if (renderer->fsaa)
			{
				primaryColorAttachment.storeAction = MTLStoreActionMultisampleResolve;
				
				id<MTLTexture> multisampleTexture = (__bridge id<MTLTexture>)(renderer->metalMultisampleTexture);
				primaryColorAttachment.texture = multisampleTexture;
				primaryColorAttachment.resolveTexture = drawable.texture;
			}
			else
			{
				primaryColorAttachment.storeAction = MTLStoreActionStore;
				primaryColorAttachment.texture = drawable.texture;
			}
			
			passDescriptor.depthAttachment.clearDepth = 1.0;
			passDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
			passDescriptor.depthAttachment.storeAction = MTLStoreActionDontCare;
			
			id<MTLTexture> depthTexture = (__bridge id<MTLTexture>)(renderer->metalDepthTexture);
			passDescriptor.depthAttachment.texture = depthTexture;
			
			id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
			id<MTLRenderCommandEncoder> renderCommandEncoder = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];
			
			[renderCommandEncoder setViewport:(MTLViewport){0.0, 0.0, (double)renderer->screenWidth, (double)renderer->screenHeight, -1.0, 1.0 }];
			renderer->metalCurrentRenderCommandEncoder = (__bridge void *)(renderCommandEncoder);
			
			drawFunc(renderer);
			
			renderer->metalCurrentRenderCommandEncoder = NULL;
			
			[renderCommandEncoder endEncoding];
			[commandBuffer presentDrawable:drawable];
			[commandBuffer commit];
		}
	}
}

TextureObject textureFromPixelData_metal(Renderer *renderer, const void *pixels, int32_t width, int32_t height)
{
	CAMetalLayer *metalLayer = (__bridge CAMetalLayer *)(renderer->metalLayer);
	id<MTLDevice> device = metalLayer.device;
	
	MTLTextureDescriptor *textureDescriptor = [[MTLTextureDescriptor alloc] init];
	textureDescriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
	textureDescriptor.width = (NSUInteger)width;
	textureDescriptor.height = (NSUInteger)height;
	
	id<MTLTexture> texture = [device newTextureWithDescriptor:textureDescriptor];
	if (texture == nil)
	{
		zgPrint("Failed to create texture in textureFromPixelData_metal");
		SDL_Quit();
	}
	
	MTLRegion region =
	{
		{ 0, 0, 0 },
		{(NSUInteger)width, (NSUInteger)height, 1}
	};
	
	NSUInteger bytesPerRow = 4 * (NSUInteger)width;
	[texture replaceRegion:region mipmapLevel:0 withBytes:pixels bytesPerRow:bytesPerRow];
	
	return (TextureObject){.metalObject = (void *)CFBridgingRetain(texture)};
}

static id<MTLBuffer> createBuffer(Renderer *renderer, const void *data, uint32_t size)
{
	CAMetalLayer *metalLayer = (__bridge CAMetalLayer *)(renderer->metalLayer);
	id<MTLDevice> device = metalLayer.device;
	
	id<MTLBuffer> buffer = [device newBufferWithBytes:data length:size options:MTLResourceStorageModeShared];
	if (buffer == nil)
	{
		zgPrint("Failed to create buffer object in createBuffer");
		SDL_Quit();
	}
	
	return buffer;
}

BufferObject createBufferObject_metal(Renderer *renderer, const void *data, uint32_t size)
{
	id<MTLBuffer> buffer = createBuffer(renderer, data, size);
	return (BufferObject){.metalObject = (void *)CFBridgingRetain(buffer)};
}

BufferArrayObject createVertexArrayObject_metal(Renderer *renderer, const void *vertices, uint32_t verticesSize)
{
	id<MTLBuffer> buffer = createBuffer(renderer, vertices, verticesSize);
	return (BufferArrayObject){.metalObject = (void *)CFBridgingRetain(buffer), .metalVerticesSize = verticesSize};
}

BufferArrayObject createVertexAndTextureCoordinateArrayObject_metal(Renderer *renderer, const void *verticesAndTextureCoordinates, uint32_t verticesSize, uint32_t textureCoordinatesSize)
{
	id<MTLBuffer> buffer = createBuffer(renderer, verticesAndTextureCoordinates, verticesSize + textureCoordinatesSize);
	return (BufferArrayObject){.metalObject = (void *)CFBridgingRetain(buffer), .metalVerticesSize = verticesSize};
}

static MTLPrimitiveType metalTypeFromRendererMode(RendererMode mode)
{
	switch (mode)
	{
		case RENDERER_TRIANGLE_MODE:
			return MTLPrimitiveTypeTriangle;
		case RENDERER_TRIANGLE_STRIP_MODE:
			return MTLPrimitiveTypeTriangleStrip;
	}
	return 0;
}

void drawVertices_metal(Renderer *renderer, mat4_t modelViewMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options)
{
//	id<MTLRenderPipelineState> pipelineState = (__bridge id<MTLRenderPipelineState>)(renderer->metalPositionPipelineState);
//	id <MTLDepthStencilState> depthStencilState = (__bridge id<MTLDepthStencilState>)(renderer->metalDepthTestEnabledPipelineState);
//
//	id<MTLRenderCommandEncoder> renderCommandEncoder = (__bridge id<MTLRenderCommandEncoder>)(renderer->metalCurrentRenderCommandEncoder);
//
//	[renderCommandEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
//	[renderCommandEncoder setRenderPipelineState:pipelineState];
//	[renderCommandEncoder setDepthStencilState:depthStencilState];
//
//	id<MTLBuffer> vertexBuffer = (__bridge id<MTLBuffer>)(vertexArrayObject.metalObject);
//	[renderCommandEncoder setVertexBuffer:vertexBuffer offset:0 atIndex:METAL_BUFFER_VERTICES_INDEX];
//
//	mat4_t modelViewProjectionMatrix = m4_mul(renderer->projectionMatrix, modelViewMatrix);
//	[renderCommandEncoder setVertexBytes:modelViewProjectionMatrix.m length:sizeof(modelViewProjectionMatrix.m) atIndex:METAL_BUFFER_MODELVIEW_PROJECTION_INDEX];
//
//	[renderCommandEncoder setFragmentBytes:&color.red length:sizeof(color) atIndex:METAL_BUFFER_COLOR_INDEX];
//
//	[renderCommandEncoder drawPrimitives:metalTypeFromRendererMode(mode) vertexStart:0 vertexCount:vertexCount];
}

void drawVerticesFromIndices_metal(Renderer *renderer, mat4_t modelViewMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options)
{
}

void drawTextureWithVertices_metal(Renderer *renderer, mat4_t modelViewMatrix, TextureObject textureObject, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options)
{
//	id<MTLRenderPipelineState> pipelineState = (__bridge id<MTLRenderPipelineState>)(renderer->metalBlendingSrcAlphaTexturePositionPipelineState);
//	id <MTLDepthStencilState> depthStencilState = (__bridge id<MTLDepthStencilState>)(renderer->metalDepthTestEnabledPipelineState);
//
//	id<MTLRenderCommandEncoder> renderCommandEncoder = (__bridge id<MTLRenderCommandEncoder>)(renderer->metalCurrentRenderCommandEncoder);
//
//	[renderCommandEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
//	[renderCommandEncoder setRenderPipelineState:pipelineState];
//	[renderCommandEncoder setDepthStencilState:depthStencilState];
//
//	id<MTLBuffer> vertexBuffer = (__bridge id<MTLBuffer>)(vertexAndTextureArrayObject.metalObject);
//	[renderCommandEncoder setVertexBuffer:vertexBuffer offset:0 atIndex:METAL_BUFFER_VERTICES_INDEX];
//
//	mat4_t modelViewProjectionMatrix = m4_mul(renderer->projectionMatrix, modelViewMatrix);
//	[renderCommandEncoder setVertexBytes:modelViewProjectionMatrix.m length:sizeof(modelViewProjectionMatrix.m) atIndex:METAL_BUFFER_MODELVIEW_PROJECTION_INDEX];
//
//	[renderCommandEncoder setFragmentBytes:&color.red length:sizeof(color) atIndex:METAL_BUFFER_COLOR_INDEX];
//
//	[renderCommandEncoder setVertexBuffer:vertexBuffer offset:vertexAndTextureArrayObject.verticesSize atIndex:METAL_BUFFER_TEXTURE_COORDINATES_INDEX];
//
//	id<MTLTexture> texture = (__bridge id<MTLTexture>)(textureObject.metalObject);
//	[renderCommandEncoder setFragmentTexture:texture atIndex:METAL_TEXTURE1_INDEX];
//
//	[renderCommandEncoder drawPrimitives:metalTypeFromRendererMode(mode) vertexStart:0 vertexCount:vertexCount];
}

void drawTextureWithVerticesFromIndices_metal(Renderer *renderer, mat4_t modelViewMatrix, TextureObject textureObject, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options)
{
	if ((options & RENDERER_OPTION_BLENDING_ALPHA) != 0 && (options & RENDERER_OPTION_DISABLE_DEPTH_TEST) == 0)
	{
		id<MTLRenderPipelineState> pipelineState = (__bridge id<MTLRenderPipelineState>)(renderer->metalBlendingSrcAlphaTexturePositionPipelineState);
		id <MTLDepthStencilState> depthStencilState = (__bridge id<MTLDepthStencilState>)(renderer->metalDepthTestEnabledPipelineState);
		
		id<MTLRenderCommandEncoder> renderCommandEncoder = (__bridge id<MTLRenderCommandEncoder>)(renderer->metalCurrentRenderCommandEncoder);
		
		[renderCommandEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
		[renderCommandEncoder setRenderPipelineState:pipelineState];
		[renderCommandEncoder setDepthStencilState:depthStencilState];
		
		id<MTLBuffer> vertexBuffer = (__bridge id<MTLBuffer>)(vertexAndTextureArrayObject.metalObject);
		[renderCommandEncoder setVertexBuffer:vertexBuffer offset:0 atIndex:METAL_BUFFER_VERTICES_INDEX];
		
		mat4_t modelViewProjectionMatrix = m4_mul(renderer->projectionMatrix, modelViewMatrix);
		[renderCommandEncoder setVertexBytes:modelViewProjectionMatrix.m length:sizeof(modelViewProjectionMatrix.m) atIndex:METAL_BUFFER_MODELVIEW_PROJECTION_INDEX];
		
		[renderCommandEncoder setFragmentBytes:&color.red length:sizeof(color) atIndex:METAL_BUFFER_COLOR_INDEX];
		
		[renderCommandEncoder setVertexBuffer:vertexBuffer offset:vertexAndTextureArrayObject.metalVerticesSize atIndex:METAL_BUFFER_TEXTURE_COORDINATES_INDEX];
		
		id<MTLTexture> texture = (__bridge id<MTLTexture>)(textureObject.metalObject);
		[renderCommandEncoder setFragmentTexture:texture atIndex:METAL_TEXTURE1_INDEX];
		
		id<MTLBuffer> indicesBuffer = (__bridge id<MTLBuffer>)(indicesBufferObject.metalObject);
		[renderCommandEncoder drawIndexedPrimitives:metalTypeFromRendererMode(mode) indexCount:indicesCount indexType:MTLIndexTypeUInt16 indexBuffer:indicesBuffer indexBufferOffset:0];
	}
}
