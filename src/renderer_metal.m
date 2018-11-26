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

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

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
	
	renderer->fsaa = fsaa;
	
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
	
	renderer->projectionMatrix = m4_mul(metalProjectionAdjustMatrix, m4_perspective(45.0f, (float)(renderer->screenWidth / renderer->screenHeight), 10.0f, 300.0f));
	
	//renderer->projectionMatrix = m4_perspective(45.0f, (float)(renderer->screenWidth / renderer->screenHeight), 10.0f, 300.0f);
	
	SDL_GetWindowSize(renderer->window, &renderer->windowWidth, &renderer->windowHeight);
	
	id<MTLDevice> device = metalLayer.device;
	
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
	// I think this is necessary; I should verify this later
	//pipelineStateDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
	//pipelineStateDescriptor.sampleCount = ...;
	
	NSError *pipelineError = nil;
	id<MTLRenderPipelineState> pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&pipelineError];
	
	if (pipelineState == nil)
	{
		NSLog(@"Pipeline state error: %@", pipelineError);
		SDL_Quit();
	}
	
	MTLDepthStencilDescriptor *depthStencilDescriptor = [MTLDepthStencilDescriptor new];
	depthStencilDescriptor.depthCompareFunction = MTLCompareFunctionEqual;
	depthStencilDescriptor.depthWriteEnabled = YES;
	
	id <MTLDepthStencilState> depthStencilState = [device newDepthStencilStateWithDescriptor:depthStencilDescriptor];
	if (depthStencilState == nil)
	{
		zgPrint("Depth stencil state failed to be created");
		SDL_Quit();
	}
	
	renderer->metalBlendingSrcAlphaTexturePositionPipelineState = (void *)CFBridgingRetain(pipelineState);
	renderer->metalDepthTestEnabledPipelineState = (void *)CFBridgingRetain(depthStencilState);
	
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
			passDescriptor.colorAttachments[0].clearColor = clearColor;
			passDescriptor.colorAttachments[0].loadAction  = MTLLoadActionClear;
			passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
			passDescriptor.colorAttachments[0].texture = drawable.texture;
			
			passDescriptor.depthAttachment.clearDepth = 1.0;
			passDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
			passDescriptor.depthAttachment.storeAction = MTLStoreActionDontCare;
			//passDescriptor.depthAttachment.texture = drawable.texture;
			
			id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
			id<MTLRenderCommandEncoder> renderCommandEncoder = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];
			
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
	return (BufferArrayObject){.metalObject = (void *)CFBridgingRetain(buffer), .verticesSize = verticesSize};
}

BufferArrayObject createVertexAndTextureCoordinateArrayObject_metal(Renderer *renderer, const void *verticesAndTextureCoordinates, uint32_t verticesSize, uint32_t textureCoordinatesSize)
{
	id<MTLBuffer> buffer = createBuffer(renderer, verticesAndTextureCoordinates, verticesSize + textureCoordinatesSize);
	return (BufferArrayObject){.metalObject = (void *)CFBridgingRetain(buffer), .verticesSize = verticesSize};
}

void drawVertices_metal(Renderer *renderer, mat4_t modelViewMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options)
{
	
}

void drawVerticesFromIndices_metal(Renderer *renderer, mat4_t modelViewMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options)
{
	
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

void drawTextureWithVertices_metal(Renderer *renderer, mat4_t modelViewMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options)
{
}

void drawTextureWithVerticesFromIndices_metal(Renderer *renderer, mat4_t modelViewMatrix, TextureObject textureObject, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options)
{
	if ((options & RENDERER_OPTION_BLENDING_ALPHA) != 0 && (options & RENDERER_OPTION_DISABLE_DEPTH_TEST) == 0)
	{
		id<MTLRenderPipelineState> pipelineState = (__bridge id<MTLRenderPipelineState>)(renderer->metalBlendingSrcAlphaTexturePositionPipelineState);
		//id <MTLDepthStencilState> depthStencilState = (__bridge id<MTLDepthStencilState>)(renderer->metalDepthTestEnabledPipelineState);
		
		id<MTLRenderCommandEncoder> renderCommandEncoder = (__bridge id<MTLRenderCommandEncoder>)(renderer->metalCurrentRenderCommandEncoder);
		
		[renderCommandEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
		[renderCommandEncoder setRenderPipelineState:pipelineState];
		//[renderCommandEncoder setDepthStencilState:depthStencilState];
		
		id<MTLBuffer> vertexBuffer = (__bridge id<MTLBuffer>)(vertexAndTextureArrayObject.metalObject);
		[renderCommandEncoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
		
		mat4_t modelViewProjectionMatrix = m4_mul(renderer->projectionMatrix, modelViewMatrix);
		[renderCommandEncoder setVertexBytes:modelViewProjectionMatrix.m length:sizeof(modelViewProjectionMatrix.m) atIndex:1];
		
		[renderCommandEncoder setVertexBuffer:vertexBuffer offset:vertexAndTextureArrayObject.verticesSize atIndex:2];
		
		[renderCommandEncoder setFragmentBytes:&color.red length:sizeof(color) atIndex:3];
		
		id<MTLTexture> texture = (__bridge id<MTLTexture>)(textureObject.metalObject);
		[renderCommandEncoder setFragmentTexture:texture atIndex:0];
		
		id<MTLBuffer> indicesBuffer = (__bridge id<MTLBuffer>)(indicesBufferObject.metalObject);
		[renderCommandEncoder drawIndexedPrimitives:metalTypeFromRendererMode(mode) indexCount:indicesCount indexType:MTLIndexTypeUInt16 indexBuffer:indicesBuffer indexBufferOffset:0];
	}
}
