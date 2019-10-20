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
#include "renderer_projection.h"
#include "quit.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Cocoa/Cocoa.h>
#import "osx.h"

// Don't use MTLPixelFormatDepth16Unorm which doesn't support 10.11 and doesn't work right on some configs (MBP11,2 / 10.13.6, but 10.14.2 works)
#define DEPTH_STENCIL_PIXEL_FORMAT MTLPixelFormatDepth32Float

static void createPipelines(Renderer *renderer);

void updateViewport_metal(Renderer *renderer, int32_t windowWidth, int32_t windowHeight);
static void updateRealViewport(Renderer *renderer);

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

// This class is mostly copied from SDL's SDL_cocoametalview to fit our own needs
@interface ZGMetalView : NSView
@end

@implementation ZGMetalView
{
	Renderer *_renderer;
}

+ (Class)layerClass
{
	return NSClassFromString(@"CAMetalLayer");
}

- (instancetype)initWithFrame:(NSRect)frame renderer:(Renderer *)renderer
{
	if ((self = [super initWithFrame:frame]))
	{
		self.wantsLayer = YES;
		self.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
		
		_renderer = renderer;
		
		[self updateDrawableSize];
	}
	
	return self;
}

- (BOOL)wantsUpdateLayer
{
	return YES;
}

- (CALayer *)makeBackingLayer
{
	return [self.class.layerClass layer];
}

- (void)updateDrawableSize
{
	CAMetalLayer *metalLayer = (CAMetalLayer *)self.layer;
	CGSize size = self.bounds.size;
	CGSize backingSize = [self convertSizeToBacking:size];
	
	metalLayer.contentsScale = backingSize.height / size.height;
	metalLayer.drawableSize = backingSize;
}

- (void)resizeWithOldSuperviewSize:(NSSize)oldSize
{
	[super resizeWithOldSuperviewSize:oldSize];
	[self updateDrawableSize];
	
	updateRealViewport(_renderer);
}

- (void)viewDidChangeBackingProperties
{
	[super viewDidChangeBackingProperties];

	if (_renderer->metalCreatedInitialPipelines)
	{
		[self updateDrawableSize];

		if (_renderer->metalWantsFsaa)
		{
			createPipelines(_renderer);
		}
		updateRealViewport(_renderer);
	}
}

@end

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

static void createAndStorePipelineState(void **pipelineStates, id<MTLDevice> device, MTLPixelFormat pixelFormat, NSArray<id<MTLFunction>> *shaderFunctions, ShaderFunctionPairIndex shaderPairIndex, PipelineOptionIndex pipelineOptionIndex, bool fsaa, uint32_t fsaaSampleCount)
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
	if (fsaaSampleCount)
	{
		pipelineStateDescriptor.sampleCount = fsaaSampleCount;
	}
	
	NSError *pipelineError = nil;
	id<MTLRenderPipelineState> pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&pipelineError];
	
	if (pipelineState == nil)
	{
		NSLog(@"Pipeline state error: %@", pipelineError);
		ZGQuit();
	}
	
	uint8_t index = pipelineIndex(shaderPairIndex, pipelineOptionIndex);
	if (pipelineStates[index] != NULL)
	{
		CFRelease(pipelineStates[index]);
	}
	pipelineStates[index] = (void *)CFBridgingRetain(pipelineState);
}

static void updateRealViewport(Renderer *renderer)
{
	CAMetalLayer *metalLayer = (__bridge CAMetalLayer *)(renderer->metalLayer);
	
	CGSize drawableSize = metalLayer.drawableSize;
	renderer->drawableWidth = (int32_t)drawableSize.width;
	renderer->drawableHeight = (int32_t)drawableSize.height;
	
	// Configure Anti Aliasing
	
	id<MTLDevice> device = metalLayer.device;
	
	id<MTLTexture> multisampleTexture;
	if (renderer->fsaa)
	{
		MTLTextureDescriptor *multisampleTextureDescriptor = [MTLTextureDescriptor new];
		multisampleTextureDescriptor.pixelFormat = metalLayer.pixelFormat;
		multisampleTextureDescriptor.width = (NSUInteger)renderer->drawableWidth;
		multisampleTextureDescriptor.height = (NSUInteger)renderer->drawableHeight;
		multisampleTextureDescriptor.resourceOptions = MTLResourceStorageModePrivate;
		multisampleTextureDescriptor.usage = MTLTextureUsageRenderTarget;
		multisampleTextureDescriptor.sampleCount = renderer->sampleCount;
		multisampleTextureDescriptor.textureType = MTLTextureType2DMultisample;
		
		multisampleTexture = [device newTextureWithDescriptor:multisampleTextureDescriptor];
	}
	else
	{
		multisampleTexture = nil;
	}
	
	// Set up depth stencil
	
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
		ZGQuit();
	}
	
	MTLTextureDescriptor *depthTextureDescriptor = [MTLTextureDescriptor new];
	depthTextureDescriptor.pixelFormat = DEPTH_STENCIL_PIXEL_FORMAT;
	depthTextureDescriptor.width = (NSUInteger)renderer->drawableWidth;
	depthTextureDescriptor.height = (NSUInteger)renderer->drawableHeight;
	depthTextureDescriptor.resourceOptions = MTLResourceStorageModePrivate;
	depthTextureDescriptor.usage = MTLTextureUsageRenderTarget;
	if (renderer->fsaa)
	{
		depthTextureDescriptor.sampleCount = renderer->sampleCount;
		depthTextureDescriptor.textureType = MTLTextureType2DMultisample;
	}
	
	id<MTLTexture> depthTexture = [device newTextureWithDescriptor:depthTextureDescriptor];
	
	// Set up render pass descriptor
	
	if (renderer->metalRenderPassDescriptor != NULL)
	{
		CFRelease(renderer->metalRenderPassDescriptor);
	}
	
	MTLClearColor clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
	
	MTLRenderPassDescriptor *renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
	
	MTLRenderPassColorAttachmentDescriptor *primaryColorAttachment = renderPassDescriptor.colorAttachments[0];
	
	primaryColorAttachment.clearColor = clearColor;
	primaryColorAttachment.loadAction  = MTLLoadActionClear;
	
	if (renderer->fsaa)
	{
		primaryColorAttachment.storeAction = MTLStoreActionMultisampleResolve;
		primaryColorAttachment.texture = multisampleTexture;
	}
	else
	{
		primaryColorAttachment.storeAction = MTLStoreActionStore;
	}
	
	renderPassDescriptor.depthAttachment.clearDepth = 1.0;
	renderPassDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
	renderPassDescriptor.depthAttachment.storeAction = MTLStoreActionDontCare;
	
	renderPassDescriptor.depthAttachment.texture = depthTexture;
	
	renderer->metalRenderPassDescriptor = (void *)CFBridgingRetain(renderPassDescriptor);
	
	renderer->metalDepthTestStencilState = (void *)CFBridgingRetain(depthStencilState);
	
	updateMetalProjectionMatrix(renderer);
}

void updateViewport_metal(Renderer *renderer, int32_t windowWidth, int32_t windowHeight)
{
	// Only update the window width/height here if necessary
	// We set up a handler if the viewport resizes independent from SDL
	if (!renderer->macosInFullscreenTransition && !renderer->fullscreen)
	{
		renderer->windowWidth = windowWidth;
		renderer->windowHeight = windowHeight;
	}
}

static void createPipelines(Renderer *renderer)
{
	CAMetalLayer *metalLayer = (__bridge CAMetalLayer *)(renderer->metalLayer);
	id<MTLDevice> device = metalLayer.device;
	
	// Prepare setting up AA
	if (renderer->metalWantsFsaa)
	{
		if (metalLayer.contentsScale >= 2.0)
		{
			if ([device supportsTextureSampleCount:MSAA_PREFERRED_RETINA_SAMPLE_COUNT])
			{
				renderer->sampleCount = MSAA_PREFERRED_RETINA_SAMPLE_COUNT;
				renderer->fsaa = true;
			}
			else
			{
				// No AA is good enough in this case for retina displays
				renderer->sampleCount = 0;
				renderer->fsaa = false;
			}
		}
		else
		{
			// All devices should support this according to the documentation
			if ([device supportsTextureSampleCount:MSAA_PREFERRED_NONRETINA_SAMPLE_COUNT])
			{
				renderer->sampleCount = MSAA_PREFERRED_NONRETINA_SAMPLE_COUNT;
				renderer->fsaa = true;
			}
		}
	}
	else
	{
		renderer->sampleCount = 0;
		renderer->fsaa = false;
	}
	
	NSArray<id<MTLFunction>> *shaderFunctions;
	if (renderer->metalShaderFunctions == NULL)
	{
		// Compile our shader function pairs
		
		id<MTLLibrary> defaultLibrary = [device newDefaultLibrary];
		if (defaultLibrary == nil)
		{
			fprintf(stderr, "Failed to find default metal library\n");
			ZGQuit();
		}
		
		NSArray<NSString *> *shaderFunctionNames = @[@"positionVertexShader", @"positionFragmentShader", @"texturePositionVertexShader", @"texturePositionFragmentShader"];
		
		NSMutableArray<id<MTLFunction>> *mutableShaderFunctions = [[NSMutableArray alloc] init];
		
		for (NSString *shaderName in shaderFunctionNames)
		{
			id<MTLFunction> function = [defaultLibrary newFunctionWithName:shaderName];
			[mutableShaderFunctions addObject:function];
		}
		
		shaderFunctions = [mutableShaderFunctions copy];
		renderer->metalShaderFunctions = (void *)CFBridgingRetain(shaderFunctions);
	}
	else
	{
		shaderFunctions = (__bridge NSArray *)renderer->metalShaderFunctions;
	}
	
	// Set up pipelines
	
	createAndStorePipelineState(renderer->metalPipelineStates, device, metalLayer.pixelFormat, shaderFunctions, SHADER_FUNCTION_POSITION_PAIR_INDEX, PIPELINE_OPTION_NONE_INDEX, renderer->fsaa, renderer->sampleCount);
	
	// renderer->metalPipelineStates[1] is unused
	
	createAndStorePipelineState(renderer->metalPipelineStates, device, metalLayer.pixelFormat, shaderFunctions, SHADER_FUNCTION_POSITION_PAIR_INDEX, PIPELINE_OPTION_BLENDING_ONE_MINUS_SOURCE_ALPHA_INDEX, renderer->fsaa, renderer->sampleCount);
	
	createAndStorePipelineState(renderer->metalPipelineStates, device, metalLayer.pixelFormat, shaderFunctions, SHADER_FUNCTION_POSITION_TEXTURE_PAIR_INDEX, PIPELINE_OPTION_NONE_INDEX, renderer->fsaa, renderer->sampleCount);
	
	createAndStorePipelineState(renderer->metalPipelineStates, device, metalLayer.pixelFormat, shaderFunctions, SHADER_FUNCTION_POSITION_TEXTURE_PAIR_INDEX, PIPELINE_OPTION_BLENDING_SOURCE_ALPHA_INDEX, renderer->fsaa, renderer->sampleCount);
	
	createAndStorePipelineState(renderer->metalPipelineStates, device, metalLayer.pixelFormat, shaderFunctions, SHADER_FUNCTION_POSITION_TEXTURE_PAIR_INDEX, PIPELINE_OPTION_BLENDING_ONE_MINUS_SOURCE_ALPHA_INDEX, renderer->fsaa, renderer->sampleCount);
	
	renderer->metalCreatedInitialPipelines = true;
}

bool createRenderer_metal(Renderer *renderer, const char *windowTitle, int32_t windowWidth, int32_t windowHeight, bool fullscreen, bool vsync, bool fsaa)
{
	@autoreleasepool
	{
		// Check if Metal is available by OS
		if (@available(macOS 10.11, *))
		{
		}
		else
		{
			return false;
		}
		
		renderer->windowWidth = windowWidth;
		renderer->windowHeight = windowHeight;
		
		renderer->metalLayer = NULL;
		renderer->metalCreatedInitialPipelines = false;
		renderer->metalRenderPassDescriptor = NULL;
		renderer->metalDepthTestStencilState = NULL;
		renderer->metalShaderFunctions = NULL;
		renderer->macosInFullscreenTransition = false;
		renderer->metalIgnoreFirstFullscreenTransition = false;
		renderer->fullscreen = false;
		
		// Find the preffered device for our game which isn't integrated, headless, or external
		// Maybe one day I will support external/removable GPUs but my game isn't very demanding
		NSArray<id<MTLDevice>> *devices = MTLCopyAllDevices();
		NSMutableArray<id<MTLDevice>> *preferredDevices = [NSMutableArray array];
		for (id<MTLDevice> device in devices)
		{
			BOOL removable;
			if (@available(macOS 10.13, *))
			{
				removable = device.removable;
			}
			else
			{
				removable = NO;
			}
			
			if (!device.lowPower && !removable && !device.headless)
			{
				[preferredDevices addObject:device];
			}
		}
		
		[preferredDevices sortUsingComparator:^NSComparisonResult(id<MTLDevice> _Nonnull device1, id<MTLDevice> _Nonnull device2) {
			uint64_t device1Score = device1.recommendedMaxWorkingSetSize;
			uint64_t device2Score = device2.recommendedMaxWorkingSetSize;
			
			if (@available(macOS 10.14, *))
			{
				device1Score += device1.maxBufferLength;
				device2Score += device2.maxBufferLength;
			}
			
			if (device1Score < device2Score)
			{
				return NSOrderedAscending;
			}
			else if (device1Score > device2Score)
			{
				return NSOrderedDescending;
			}
			else
			{
				return NSOrderedSame;
			}
		}];
		
		id<MTLDevice> preferredDevice = preferredDevices.lastObject;
		
		id<MTLDevice> device = (preferredDevice != nil) ? preferredDevice : MTLCreateSystemDefaultDevice();
		if (device == nil)
		{
			fprintf(stderr, "Error: Failed to create metal device!\n");
			return false;
		}
		
		uint32_t videoFlags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
		renderer->window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, videoFlags);
		
		if (renderer->window == NULL)
		{
			return false;
		}
		
		SDL_SysWMinfo systemInfo;
		SDL_VERSION(&systemInfo.version);
		SDL_GetWindowWMInfo(renderer->window, &systemInfo);
		
		NSWindow *window = systemInfo.info.cocoa.window;
		NSView *contentView = window.contentView;
		
		// Add our metal view ourselves
		// Don't create metal layer/view by creating a SDL_Renderer
		// because it does a bunch of stuff we don't need for SDL's own renderer
		ZGMetalView *metalView = [[ZGMetalView alloc] initWithFrame:contentView.frame renderer:renderer];
		[contentView addSubview:metalView];
		
		CAMetalLayer *metalLayer = (CAMetalLayer *)metalView.layer;
		metalLayer.device = device;
		// Don't set framebufferOnly to NO like SDL does for its own renderer.
		// It does that to support an option (at cost of potential performance) that we don't need.
		
		if (@available(macOS 10.13, *))
		{
			metalLayer.displaySyncEnabled = vsync;
			renderer->vsync = vsync;
		}
		else
		{
			renderer->vsync = true;
		}
		
		// Create pipelines
		renderer->metalWantsFsaa = fsaa;
		renderer->metalLayer = (void *)CFBridgingRetain(metalLayer);
		memset(renderer->metalPipelineStates, 0, sizeof(renderer->metalPipelineStates));
		createPipelines(renderer);
		
		// Update view port
		updateRealViewport(renderer);
		
		// Set up remaining renderer properties
		
		id<MTLCommandQueue> queue = [device newCommandQueue];
		renderer->metalCommandQueue = (void *)CFBridgingRetain(queue);
		renderer->metalCurrentRenderCommandEncoder = NULL;
		
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
		
		// Register for full screen notifications
		renderer->metalIgnoreFirstFullscreenTransition = fullscreen;
		registerForNativeFullscreenEvents((__bridge void *)window, renderer, fullscreen);
	}
	
	return true;
}

void renderFrame_metal(Renderer *renderer, void (*drawFunc)(Renderer *))
{
	if (renderer->metalIgnoreFirstFullscreenTransition)
	{
		if (renderer->fullscreen)
		{
			renderer->metalIgnoreFirstFullscreenTransition = false;
		}
		else
		{
			return;
		}
	}

	@autoreleasepool
	{
		CAMetalLayer *metalLayer = (__bridge CAMetalLayer *)(renderer->metalLayer);
		id<MTLCommandQueue> queue = (__bridge id<MTLCommandQueue>)(renderer->metalCommandQueue);
		
		id<CAMetalDrawable> drawable = [metalLayer nextDrawable];
		
		if (drawable != nil)
		{
			id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
			
			MTLRenderPassDescriptor *renderPassDescriptor = (__bridge MTLRenderPassDescriptor *)renderer->metalRenderPassDescriptor;
			
			MTLRenderPassColorAttachmentDescriptor *primaryColorAttachment = renderPassDescriptor.colorAttachments[0];
			
			if (renderer->fsaa)
			{
				primaryColorAttachment.resolveTexture = drawable.texture;
			}
			else
			{
				primaryColorAttachment.texture = drawable.texture;
			}
			
			id<MTLRenderCommandEncoder> renderCommandEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
			
			[renderCommandEncoder setViewport:(MTLViewport){0.0, 0.0, (double)renderer->drawableWidth, (double)renderer->drawableHeight, -1.0, 1.0 }];
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
	textureDescriptor.pixelFormat = MTLPixelFormatBGRA8Unorm;
	textureDescriptor.width = (NSUInteger)width;
	textureDescriptor.height = (NSUInteger)height;
	
	id<MTLTexture> texture = [device newTextureWithDescriptor:textureDescriptor];
	if (texture == nil)
	{
		fprintf(stderr, "Failed to create texture in textureFromPixelData_metal\n");
		ZGQuit();
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
		ZGQuit();
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
