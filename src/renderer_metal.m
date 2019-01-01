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

#include "metal_indices.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

// Don't use MTLPixelFormatDepth16Unorm which doesn't work right on some machines (MBP11,2 / 10.13.6)
#define DEPTH_STENCIL_PIXEL_FORMAT MTLPixelFormatDepth32Float

static void updateViewport_metal(Renderer *renderer);

void renderFrame_metal(Renderer *renderer, void (*drawFunc)(Renderer *));

TextureObject textureFromPixelData_metal(Renderer *renderer, const void *pixels, int32_t width, int32_t height);

void deleteTexture_metal(Renderer *renderer, TextureObject texture);

BufferObject createBufferObject_metal(Renderer *renderer, const void *data, uint32_t size);

BufferArrayObject createVertexArrayObject_metal(Renderer *renderer, const void *vertices, uint32_t verticesSize);

BufferArrayObject createVertexAndTextureCoordinateArrayObject_metal(Renderer *renderer, const void *verticesAndTextureCoordinates, uint32_t verticesSize, uint32_t textureCoordinatesSize);

void drawVertices_metal(Renderer *renderer, float *modelViewProjectionMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options);

void drawVerticesFromIndices_metal(Renderer *renderer, float *modelViewProjectionMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options);

void drawTextureWithVertices_metal(Renderer *renderer, float *modelViewProjectionMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options);

void drawTextureWithVerticesFromIndices_metal(Renderer *renderer, float *modelViewProjectionMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options);

// If this changes, make sure to change MAX_PIPELINE_COUNT
typedef enum
{
	SHADER_FUNCTION_POSITION_PAIR_INDEX = 0,
	SHADER_FUNCTION_POSITION_TEXTURE_PAIR_INDEX = 1
} ShaderFunctionPairIndex;

// If this changes, make sure to change MAX_PIPELINE_COUNT
typedef enum
{
	PIPELINE_OPTION_NONE_INDEX = 0,
	PIPELINE_OPTION_BLENDING_SOURCE_ALPHA_INDEX = 1,
	PIPELINE_OPTION_BLENDING_ONE_MINUS_SOURCE_ALPHA_INDEX = 2
} PipelineOptionIndex;

static uint8_t pipelineIndex(ShaderFunctionPairIndex shaderFunctionPairIndex, PipelineOptionIndex pipelineOptionIndex)
{
	return shaderFunctionPairIndex * 3 + pipelineOptionIndex;
}

static PipelineOptionIndex rendererOptionsToPipelineOptionIndex(RendererOptions rendererOptions)
{
	if ((rendererOptions & RENDERER_OPTION_BLENDING_ALPHA) != 0)
	{
		return PIPELINE_OPTION_BLENDING_SOURCE_ALPHA_INDEX;
	}
	else if ((rendererOptions & RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA) != 0)
	{
		return PIPELINE_OPTION_BLENDING_ONE_MINUS_SOURCE_ALPHA_INDEX;
	}
	else
	{
		return PIPELINE_OPTION_NONE_INDEX;
	}
}

static void createAndStorePipelineState(void **pipelineStates, id<MTLDevice> device, MTLPixelFormat pixelFormat, NSArray<id<MTLFunction>> *shaderFunctions, ShaderFunctionPairIndex shaderPairIndex, PipelineOptionIndex pipelineOptionIndex, SDL_bool fsaa)
{
	MTLRenderPipelineDescriptor *pipelineStateDescriptor = [MTLRenderPipelineDescriptor new];
	pipelineStateDescriptor.vertexFunction = shaderFunctions[shaderPairIndex * 2];
	pipelineStateDescriptor.fragmentFunction = shaderFunctions[shaderPairIndex * 2 + 1];
	
	pipelineStateDescriptor.colorAttachments[0].pixelFormat = pixelFormat;
	
	if (pipelineOptionIndex == PIPELINE_OPTION_BLENDING_SOURCE_ALPHA_INDEX || pipelineOptionIndex == PIPELINE_OPTION_BLENDING_ONE_MINUS_SOURCE_ALPHA_INDEX)
	{
		pipelineStateDescriptor.colorAttachments[0].blendingEnabled = YES;
		
		MTLBlendFactor destinationBlendFactor = (pipelineOptionIndex == PIPELINE_OPTION_BLENDING_SOURCE_ALPHA_INDEX) ? MTLBlendFactorSourceAlpha : MTLBlendFactorOneMinusSourceAlpha;
		
		pipelineStateDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
		pipelineStateDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
		
		pipelineStateDescriptor.colorAttachments[0].destinationRGBBlendFactor = destinationBlendFactor;
		pipelineStateDescriptor.colorAttachments[0].destinationAlphaBlendFactor = destinationBlendFactor;
	}
	
	pipelineStateDescriptor.depthAttachmentPixelFormat = DEPTH_STENCIL_PIXEL_FORMAT;
	if (fsaa)
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
	
	pipelineStates[pipelineIndex(shaderPairIndex, pipelineOptionIndex)] = (void *)CFBridgingRetain(pipelineState);
}

static void updateViewport_metal(Renderer *renderer)
{
	CAMetalLayer *metalLayer = (__bridge CAMetalLayer *)(renderer->metalLayer);
	
	CGSize drawableSize = metalLayer.drawableSize;
	renderer->screenWidth = (int32_t)drawableSize.width;
	renderer->screenHeight = (int32_t)drawableSize.height;
	
	// Configure Anti Aliasing
	
	if (renderer->metalMultisampleTexture != NULL)
	{
		CFRelease(renderer->metalMultisampleTexture);
	}
	
	id<MTLDevice> device = metalLayer.device;
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
	
	// Set up depth stencil
	
	if (renderer->metalDepthTexture != NULL)
	{
		CFRelease(renderer->metalDepthTexture);
	}
	
	if (renderer->metalDepthTestStencilState != NULL)
	{
		CFRelease(renderer->metalDepthTestStencilState);
	}
	
	MTLDepthStencilDescriptor *depthStencilDescriptor = [MTLDepthStencilDescriptor new];
	depthStencilDescriptor.depthCompareFunction = MTLCompareFunctionLessEqual;
	depthStencilDescriptor.depthWriteEnabled = YES;
	
	id<MTLDepthStencilState> depthStencilState = [device newDepthStencilStateWithDescriptor:depthStencilDescriptor];
	if (depthStencilState == nil)
	{
		fprintf(stderr, "Depth stencil state failed to be created\n");
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
	renderer->metalDepthTestStencilState = (void *)CFBridgingRetain(depthStencilState);
}

SDL_bool createRenderer_metal(Renderer *renderer, const char *windowTitle, int32_t windowWidth, int32_t windowHeight, uint32_t videoFlags, SDL_bool vsync, SDL_bool fsaa)
{
	@autoreleasepool
	{
		// Check if Metal is available by OS
		if (@available(macOS 10.11, *))
		{
		}
		else
		{
			return SDL_FALSE;
		}
		
		SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
		
		renderer->window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, videoFlags);
		
		if (renderer->window == NULL)
		{
			return SDL_FALSE;
		}
		
		SDL_RendererFlags sdlRenderFlags = vsync ? SDL_RENDERER_PRESENTVSYNC : 0;
		SDL_Renderer *sdlRenderer = SDL_CreateRenderer(renderer->window, -1, sdlRenderFlags);
		if (sdlRenderer == NULL)
		{
			SDL_DestroyWindow(renderer->window);
			return SDL_FALSE;
		}
		
		SDL_RendererInfo rendererInfo;
		if (SDL_GetRendererInfo(sdlRenderer, &rendererInfo) < 0)
		{
			SDL_DestroyRenderer(sdlRenderer);
			SDL_DestroyWindow(renderer->window);
			return SDL_FALSE;
		}
		
		if (strcmp(rendererInfo.name, "metal") != 0)
		{
			SDL_DestroyRenderer(sdlRenderer);
			SDL_DestroyWindow(renderer->window);
			return SDL_FALSE;
		}
		
		CAMetalLayer *metalLayer = (__bridge CAMetalLayer *)(SDL_RenderGetMetalLayer(sdlRenderer));
		if (metalLayer == nil)
		{
			SDL_DestroyRenderer(sdlRenderer);
			SDL_DestroyWindow(renderer->window);
			return SDL_FALSE;
		}
		
		SDL_DestroyRenderer(sdlRenderer);
		
		if (@available(macOS 10.13, *))
		{
			renderer->vsync = metalLayer.displaySyncEnabled;
		}
		else
		{
			renderer->vsync = vsync;
		}
		
		SDL_GetWindowSize(renderer->window, &renderer->windowWidth, &renderer->windowHeight);
		
		id<MTLDevice> device = metalLayer.device;
		
		// Update view port
		
		renderer->fsaa = (fsaa && [device supportsTextureSampleCount:MSAA_SAMPLE_COUNT]);
		renderer->metalLayer = (void *)CFBridgingRetain(metalLayer);
		renderer->metalDepthTexture = NULL;
		renderer->metalDepthTestStencilState = NULL;
		renderer->metalMultisampleTexture = NULL;
		updateViewport_metal(renderer);
		
		// Compile our shader function pairs
		
		id<MTLLibrary> defaultLibrary = [device newDefaultLibrary];
		if (defaultLibrary == nil)
		{
			fprintf(stderr, "Failed to find default metal library\n");
			SDL_Quit();
		}
		
		NSArray<NSString *> *shaderFunctionNames = @[@"positionVertexShader", @"positionFragmentShader", @"texturePositionVertexShader", @"texturePositionFragmentShader"];
		
		NSMutableArray<id<MTLFunction>> *shaderFunctions = [[NSMutableArray alloc] init];
		
		for (NSString *shaderName in shaderFunctionNames)
		{
			id<MTLFunction> function = [defaultLibrary newFunctionWithName:shaderName];
			[shaderFunctions addObject:function];
		}
		
		// Set up pipelines
		
		createAndStorePipelineState(renderer->metalPipelineStates, device, metalLayer.pixelFormat, shaderFunctions, SHADER_FUNCTION_POSITION_PAIR_INDEX, PIPELINE_OPTION_NONE_INDEX, renderer->fsaa);
		
		// Unused pipeline
		renderer->metalPipelineStates[1] = NULL;
		
		createAndStorePipelineState(renderer->metalPipelineStates, device, metalLayer.pixelFormat, shaderFunctions, SHADER_FUNCTION_POSITION_PAIR_INDEX, PIPELINE_OPTION_BLENDING_ONE_MINUS_SOURCE_ALPHA_INDEX, renderer->fsaa);
		
		createAndStorePipelineState(renderer->metalPipelineStates, device, metalLayer.pixelFormat, shaderFunctions, SHADER_FUNCTION_POSITION_TEXTURE_PAIR_INDEX, PIPELINE_OPTION_NONE_INDEX, renderer->fsaa);
		
		createAndStorePipelineState(renderer->metalPipelineStates, device, metalLayer.pixelFormat, shaderFunctions, SHADER_FUNCTION_POSITION_TEXTURE_PAIR_INDEX, PIPELINE_OPTION_BLENDING_SOURCE_ALPHA_INDEX, renderer->fsaa);
		
		createAndStorePipelineState(renderer->metalPipelineStates, device, metalLayer.pixelFormat, shaderFunctions, SHADER_FUNCTION_POSITION_TEXTURE_PAIR_INDEX, PIPELINE_OPTION_BLENDING_ONE_MINUS_SOURCE_ALPHA_INDEX, renderer->fsaa);
		
		// Set up remaining renderer properties
		
		id<MTLCommandQueue> queue = [device newCommandQueue];
		renderer->metalCommandQueue = (void *)CFBridgingRetain(queue);
		renderer->metalCurrentRenderCommandEncoder = NULL;
		
		renderer->ndcType = NDC_TYPE_METAL;
		
		renderer->updateViewportPtr = updateViewport_metal;
		renderer->renderFramePtr = renderFrame_metal;
		renderer->textureFromPixelDataPtr = textureFromPixelData_metal;
		renderer->deleteTexturePtr = deleteTexture_metal;
		renderer->createBufferObjectPtr = createBufferObject_metal;
		renderer->createVertexArrayObjectPtr = createVertexArrayObject_metal;
		renderer->createVertexAndTextureCoordinateArrayObjectPtr = createVertexAndTextureCoordinateArrayObject_metal;
		renderer->drawVerticesPtr = drawVertices_metal;
		renderer->drawVerticesFromIndicesPtr = drawVerticesFromIndices_metal;
		renderer->drawTextureWithVerticesPtr = drawTextureWithVertices_metal;
		renderer->drawTextureWithVerticesFromIndicesPtr = drawTextureWithVerticesFromIndices_metal;
	}
	
	return SDL_TRUE;
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
		fprintf(stderr, "Failed to create texture in textureFromPixelData_metal\n");
		SDL_Quit();
	}
	
	MTLRegion region = MTLRegionMake2D(0, 0, (NSUInteger)width, (NSUInteger)height);
	
	NSUInteger bytesPerRow = 4 * (NSUInteger)width;
	[texture replaceRegion:region mipmapLevel:0 withBytes:pixels bytesPerRow:bytesPerRow];
	
	return (TextureObject){.metalObject = (void *)CFBridgingRetain(texture)};
}

void deleteTexture_metal(Renderer *renderer, TextureObject textureObject)
{
	CFRelease(textureObject.metalObject);
}

static id<MTLBuffer> createBuffer(Renderer *renderer, const void *data, uint32_t size)
{
	CAMetalLayer *metalLayer = (__bridge CAMetalLayer *)(renderer->metalLayer);
	id<MTLDevice> device = metalLayer.device;
	
	id<MTLBuffer> buffer = [device newBufferWithBytes:data length:size options:MTLResourceStorageModeShared];
	if (buffer == nil)
	{
		fprintf(stderr, "Failed to create buffer object in createBuffer\n");
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

static void encodeVertexState(id<MTLRenderCommandEncoder> renderCommandEncoder, Renderer *renderer, ShaderFunctionPairIndex shaderFunctionPairIndex, id<MTLBuffer> vertexBuffer, RendererOptions options)
{
	PipelineOptionIndex pipelineOptionIndex = rendererOptionsToPipelineOptionIndex(options);
	
	id<MTLRenderPipelineState> pipelineState = (__bridge id<MTLRenderPipelineState>)(renderer->metalPipelineStates[pipelineIndex(shaderFunctionPairIndex, pipelineOptionIndex)]);
	
	[renderCommandEncoder setRenderPipelineState:pipelineState];
	
	if ((options & RENDERER_OPTION_DISABLE_DEPTH_TEST) == 0)
	{
		id <MTLDepthStencilState> depthStencilState = (__bridge id<MTLDepthStencilState>)(renderer->metalDepthTestStencilState);
		[renderCommandEncoder setDepthStencilState:depthStencilState];
	}
	
	[renderCommandEncoder setVertexBuffer:vertexBuffer offset:0 atIndex:METAL_BUFFER_VERTICES_INDEX];
}

static void encodeModelViewMatrixAndColor(id<MTLRenderCommandEncoder> renderCommandEncoder, Renderer *renderer, float *modelViewProjectionMatrix, color4_t color)
{
	[renderCommandEncoder setVertexBytes:modelViewProjectionMatrix length:sizeof(*modelViewProjectionMatrix) * 16 atIndex:METAL_BUFFER_MODELVIEW_PROJECTION_INDEX];
	
	[renderCommandEncoder setFragmentBytes:&color.red length:sizeof(color) atIndex:METAL_BUFFER_COLOR_INDEX];
}

static void encodeVertexAndTextureState(id<MTLRenderCommandEncoder> renderCommandEncoder, Renderer *renderer, TextureObject textureObject, BufferArrayObject vertexAndTextureArrayObject, RendererOptions options)
{
	id<MTLBuffer> vertexAndTextureBuffer = (__bridge id<MTLBuffer>)(vertexAndTextureArrayObject.metalObject);
	encodeVertexState(renderCommandEncoder, renderer, SHADER_FUNCTION_POSITION_TEXTURE_PAIR_INDEX, vertexAndTextureBuffer, options);
	
	[renderCommandEncoder setVertexBuffer:vertexAndTextureBuffer offset:vertexAndTextureArrayObject.metalVerticesSize atIndex:METAL_BUFFER_TEXTURE_COORDINATES_INDEX];
	
	id<MTLTexture> texture = (__bridge id<MTLTexture>)(textureObject.metalObject);
	[renderCommandEncoder setFragmentTexture:texture atIndex:METAL_TEXTURE_INDEX];
}

void drawVertices_metal(Renderer *renderer, float *modelViewProjectionMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options)
{
	id<MTLRenderCommandEncoder> renderCommandEncoder = (__bridge id<MTLRenderCommandEncoder>)(renderer->metalCurrentRenderCommandEncoder);
	
	id<MTLBuffer> vertexBuffer = (__bridge id<MTLBuffer>)(vertexArrayObject.metalObject);
	
	encodeVertexState(renderCommandEncoder, renderer, SHADER_FUNCTION_POSITION_PAIR_INDEX, vertexBuffer, options);
	
	encodeModelViewMatrixAndColor(renderCommandEncoder, renderer, modelViewProjectionMatrix, color);
	
	[renderCommandEncoder drawPrimitives:metalTypeFromRendererMode(mode) vertexStart:0 vertexCount:vertexCount];
}

void drawVerticesFromIndices_metal(Renderer *renderer, float *modelViewProjectionMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options)
{
	id<MTLRenderCommandEncoder> renderCommandEncoder = (__bridge id<MTLRenderCommandEncoder>)(renderer->metalCurrentRenderCommandEncoder);
	
	id<MTLBuffer> vertexBuffer = (__bridge id<MTLBuffer>)(vertexArrayObject.metalObject);
	
	encodeVertexState(renderCommandEncoder, renderer, SHADER_FUNCTION_POSITION_PAIR_INDEX, vertexBuffer, options);
	
	encodeModelViewMatrixAndColor(renderCommandEncoder, renderer, modelViewProjectionMatrix, color);
	
	id<MTLBuffer> indicesBuffer = (__bridge id<MTLBuffer>)(indicesBufferObject.metalObject);
	[renderCommandEncoder drawIndexedPrimitives:metalTypeFromRendererMode(mode) indexCount:indicesCount indexType:MTLIndexTypeUInt16 indexBuffer:indicesBuffer indexBufferOffset:0];
}

void drawTextureWithVertices_metal(Renderer *renderer, float *modelViewProjectionMatrix, TextureObject textureObject, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options)
{
	id<MTLRenderCommandEncoder> renderCommandEncoder = (__bridge id<MTLRenderCommandEncoder>)(renderer->metalCurrentRenderCommandEncoder);
	
	encodeVertexAndTextureState(renderCommandEncoder, renderer, textureObject, vertexAndTextureArrayObject, options);
	
	encodeModelViewMatrixAndColor(renderCommandEncoder, renderer, modelViewProjectionMatrix, color);
	
	[renderCommandEncoder drawPrimitives:metalTypeFromRendererMode(mode) vertexStart:0 vertexCount:vertexCount];
}

void drawTextureWithVerticesFromIndices_metal(Renderer *renderer, float *modelViewProjectionMatrix, TextureObject textureObject, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options)
{
	id<MTLRenderCommandEncoder> renderCommandEncoder = (__bridge id<MTLRenderCommandEncoder>)(renderer->metalCurrentRenderCommandEncoder);
	
	encodeVertexAndTextureState(renderCommandEncoder, renderer, textureObject, vertexAndTextureArrayObject, options);
	
	encodeModelViewMatrixAndColor(renderCommandEncoder, renderer, modelViewProjectionMatrix, color);
	
	id<MTLBuffer> indicesBuffer = (__bridge id<MTLBuffer>)(indicesBufferObject.metalObject);
	[renderCommandEncoder drawIndexedPrimitives:metalTypeFromRendererMode(mode) indexCount:indicesCount indexType:MTLIndexTypeUInt16 indexBuffer:indicesBuffer indexBufferOffset:0];
}
