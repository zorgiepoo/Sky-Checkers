/*
 * Copyright 2019 Mayur Pawashe
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

#include "renderer_d3d11.h"
#include "renderer_types.h"
#include "renderer_projection.h"

#include <d3d11.h>
#include <d3dcompiler.h>

extern "C" static void updateViewport_d3d11(Renderer *renderer);

extern "C" void renderFrame_d3d11(Renderer *renderer, void(*drawFunc)(Renderer *));

extern "C" TextureObject textureFromPixelData_d3d11(Renderer *renderer, const void *pixels, int32_t width, int32_t height);

extern "C" void deleteTexture_d3d11(Renderer *renderer, TextureObject texture);

extern "C" BufferObject createBufferObject_d3d11(Renderer *renderer, const void *data, uint32_t size);

extern "C" BufferArrayObject createVertexArrayObject_d3d11(Renderer *renderer, const void *vertices, uint32_t verticesSize);

extern "C" BufferArrayObject createVertexAndTextureCoordinateArrayObject_d3d11(Renderer *renderer, const void *verticesAndTextureCoordinates, uint32_t verticesSize, uint32_t textureCoordinatesSize);

extern "C" void drawVertices_d3d11(Renderer *renderer, float *modelViewProjectionMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options);

extern "C" void drawVerticesFromIndices_d3d11(Renderer *renderer, float *modelViewProjectionMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options);

extern "C" void drawTextureWithVertices_d3d11(Renderer *renderer, float *modelViewProjectionMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options);

extern "C" void drawTextureWithVerticesFromIndices_d3d11(Renderer *renderer, float *modelViewProjectionMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options);

extern "C" static void updateViewport_d3d11(Renderer *renderer)
{
	renderer->drawableWidth = renderer->windowWidth;
	renderer->drawableHeight = renderer->windowHeight;

	IDXGISwapChain *swapChain = (IDXGISwapChain *)renderer->swapChain;
	ID3D11DeviceContext *context = (ID3D11DeviceContext *)renderer->context;

	// Release previous render target view and resize swapchain if necessary
	ID3D11RenderTargetView *renderTargetView = (ID3D11RenderTargetView *)renderer->renderTargetView;
	if (renderTargetView != nullptr)
	{
		context->OMSetRenderTargets(0, nullptr, nullptr);

		ID3D11Texture2D *depthStencilBuffer = (ID3D11Texture2D *)renderer->depthStencilBuffer;
		depthStencilBuffer->Release();

		ID3D11DepthStencilView *depthStencilView = (ID3D11DepthStencilView *)renderer->depthStencilView;
		depthStencilView->Release();

		renderTargetView->Release();
		renderTargetView = nullptr;

		HRESULT resizeResult = swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
		if (FAILED(resizeResult))
		{
			fprintf(stderr, "Error: Failed to resize swapchain buffers\n");
			abort();
		}
	}

	// Get back buffer
	ID3D11Texture2D *backBufferPtr = nullptr;
	HRESULT getBackBufferResult = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID *)&backBufferPtr);
	if (FAILED(getBackBufferResult))
	{
		fprintf(stderr, "Error: Failed to get back buffer: %d\n", getBackBufferResult);
		abort();
	}

	// Create render target view with back buffer
	ID3D11Device *device = (ID3D11Device *)renderer->device;
	HRESULT targetViewResult = device->CreateRenderTargetView(backBufferPtr, NULL, &renderTargetView);
	backBufferPtr->Release();
	if (FAILED(targetViewResult))
	{
		fprintf(stderr, "Error: Failed to create render target view: %d\n", targetViewResult);
		abort();
	}

	// Create depth buffer
	ID3D11Texture2D *depthStencilBuffer = nullptr;

	D3D11_TEXTURE2D_DESC depthBufferDescription;
	ZeroMemory(&depthBufferDescription, sizeof(depthBufferDescription));

	depthBufferDescription.Width = renderer->drawableWidth;
	depthBufferDescription.Height = renderer->drawableHeight;
	depthBufferDescription.MipLevels = 1;
	depthBufferDescription.ArraySize = 1;
	depthBufferDescription.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	// Assume no anti-aliasing for now
	depthBufferDescription.SampleDesc.Count = 1;
	depthBufferDescription.SampleDesc.Quality = 0;
	depthBufferDescription.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDescription.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthBufferDescription.CPUAccessFlags = 0;
	depthBufferDescription.MiscFlags = 0;

	HRESULT depthTextureResult = device->CreateTexture2D(&depthBufferDescription, NULL, &depthStencilBuffer);
	if (FAILED(depthTextureResult))
	{
		fprintf(stderr, "Error: Failed to create D3D11 depth texture: %d\n", depthTextureResult);
		abort();
	}

	// Create depth stencil view
	ID3D11DepthStencilView *depthStencilView = nullptr;

	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDescription;
	ZeroMemory(&depthStencilViewDescription, sizeof(depthStencilViewDescription));

	depthStencilViewDescription.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDescription.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDescription.Texture2D.MipSlice = 0;

	HRESULT depthStencilViewResult = device->CreateDepthStencilView(depthStencilBuffer, &depthStencilViewDescription, &depthStencilView);
	if (FAILED(depthStencilViewResult))
	{
		fprintf(stderr, "Error: Failed to create D3D11 depth stencil view: %d\n", depthStencilViewResult);
		abort();
	}

	// Bind render target view and depth stencil buffer
	context->OMSetRenderTargets(1, &renderTargetView, depthStencilView);

	// Set up viewport
	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(viewport));

	viewport.Width = (float)renderer->drawableWidth;
	viewport.Height = (float)renderer->drawableHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	context->RSSetViewports(1, &viewport);

	updateProjectionMatrix(renderer);

	renderer->renderTargetView = renderTargetView;
	renderer->depthStencilView = depthStencilView;
	renderer->depthStencilBuffer = depthStencilBuffer;
}

extern "C" SDL_bool createRenderer_d3d11(Renderer *renderer, const char *windowTitle, int32_t windowWidth, int32_t windowHeight, SDL_bool fullscreen, SDL_bool vsync, SDL_bool fsaa)
{
	// Need to initialize D3D states here in order to goto INIT_FAILURE on failure
	ID3D11Device *device = nullptr;
	IDXGISwapChain *swapChain = nullptr;
	ID3D11DeviceContext *context = nullptr;
	ID3D11DepthStencilState *disabledDepthStencilState = nullptr;
	ID3D11RasterizerState *rasterState = nullptr;

	uint32_t videoFlags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;

	renderer->windowWidth = max(windowWidth, 1);
	renderer->windowHeight = max(windowHeight, 1);

	renderer->window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, videoFlags);
	if (renderer->window == NULL)
	{
		goto INIT_FAILURE;
	}

	SDL_SetWindowMinimumSize(renderer->window, 8, 8);

	SDL_GetWindowSize(renderer->window, &renderer->windowWidth, &renderer->windowHeight);

	renderer->fullscreen = SDL_FALSE;

	IDXGIAdapter *adapter = NULL; // use default adapter for now
	UINT deviceFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#ifdef _DEBUG
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	renderer->vsync = SDL_FALSE;

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	// Infer window width/height from our SDL window
	swapChainDesc.BufferDesc.Width = 0;
	swapChainDesc.BufferDesc.Height = 0;

	// This is assuming no vsync, figure out the vsync case later
	// We have to query for adapter's refresh rate I guess..
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;

	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Assume no anti-aliasing for now
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// Use one buffer for now, figure out fullscreen later
	swapChainDesc.BufferCount = 1;

	// figure out fullscreen later
	swapChainDesc.Windowed = !renderer->fullscreen;

	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// Will avoid setting flags for now, may need to revisit later for fullscreen
	swapChainDesc.Flags = 0;

	SDL_SysWMinfo systemInfo;
	SDL_VERSION(&systemInfo.version);
	SDL_GetWindowWMInfo(renderer->window, &systemInfo);

	swapChainDesc.OutputWindow = systemInfo.info.win.window;

	// I should request for DX 10 and 9 feature levels too
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

	HRESULT deviceResult = D3D11CreateDeviceAndSwapChain(adapter, D3D_DRIVER_TYPE_HARDWARE, nullptr, deviceFlags, &featureLevel, 1, D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &device, nullptr, &context);
	if (FAILED(deviceResult))
	{
		fprintf(stderr, "Error: Failed to create D3D11 device: %d\n", deviceResult);
		swapChain = nullptr;
		device = nullptr;
		context = nullptr;

		goto INIT_FAILURE;
	}

	// Create disabled depth stencil state
	// D3D already has a default enabled depth stencil state that suits our needs
	D3D11_DEPTH_STENCIL_DESC disabledDepthStencilDescription;
	ZeroMemory(&disabledDepthStencilDescription, sizeof(disabledDepthStencilDescription));

	disabledDepthStencilDescription.DepthEnable = FALSE;
	disabledDepthStencilDescription.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	disabledDepthStencilDescription.DepthFunc = D3D11_COMPARISON_LESS;
	disabledDepthStencilDescription.StencilEnable = TRUE;
	disabledDepthStencilDescription.StencilReadMask = 0xFF;
	disabledDepthStencilDescription.StencilWriteMask = 0xFF;
	disabledDepthStencilDescription.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	disabledDepthStencilDescription.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	disabledDepthStencilDescription.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	disabledDepthStencilDescription.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	disabledDepthStencilDescription.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	disabledDepthStencilDescription.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	disabledDepthStencilDescription.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	disabledDepthStencilDescription.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	
	HRESULT depthStencilResult = device->CreateDepthStencilState(&disabledDepthStencilDescription, &disabledDepthStencilState);
	if (FAILED(depthStencilResult))
	{
		fprintf(stderr, "Error: Failed to create D3D11 depth stencil: %d\n", depthStencilResult);
		disabledDepthStencilState = nullptr;
		goto INIT_FAILURE;
	}

	// Set up raster description
	D3D11_RASTERIZER_DESC rasterDescription;
	ZeroMemory(&rasterDescription, sizeof(rasterDescription));

	rasterDescription.AntialiasedLineEnable = FALSE;
	rasterDescription.CullMode = D3D11_CULL_NONE;
	rasterDescription.DepthBias = 0;
	rasterDescription.DepthBiasClamp = 0.0f;
	rasterDescription.DepthClipEnable = TRUE;
	rasterDescription.FillMode = D3D11_FILL_SOLID;
	rasterDescription.FrontCounterClockwise = FALSE;
	rasterDescription.MultisampleEnable = FALSE;
	rasterDescription.ScissorEnable = FALSE;
	rasterDescription.SlopeScaledDepthBias = 0.0f;
	
	HRESULT rasterizerStateResult = device->CreateRasterizerState(&rasterDescription, &rasterState);
	if (FAILED(rasterizerStateResult))
	{
		fprintf(stderr, "Error: Failed to create raster state: %d\n", rasterizerStateResult);
		rasterState = nullptr;
		goto INIT_FAILURE;
	}

	context->RSSetState(rasterState);

	renderer->ndcType = NDC_TYPE_METAL;
	renderer->device = device;
	renderer->context = context;
	renderer->swapChain = swapChain;

	renderer->renderTargetView = nullptr;
	renderer->depthStencilView = nullptr;
	renderer->depthStencilBuffer = nullptr;
	updateViewport_d3d11(renderer);

	renderer->disabledDepthStencilState = disabledDepthStencilState;

	renderer->updateViewportPtr = updateViewport_d3d11;
	renderer->renderFramePtr = renderFrame_d3d11;
	renderer->textureFromPixelDataPtr = textureFromPixelData_d3d11;
	renderer->deleteTexturePtr = deleteTexture_d3d11;
	renderer->createBufferObjectPtr = createBufferObject_d3d11;
	renderer->createVertexArrayObjectPtr = createVertexArrayObject_d3d11;
	renderer->createVertexAndTextureCoordinateArrayObjectPtr = createVertexAndTextureCoordinateArrayObject_d3d11;
	renderer->drawVerticesPtr = drawVertices_d3d11;
	renderer->drawVerticesFromIndicesPtr = drawVerticesFromIndices_d3d11;
	renderer->drawTextureWithVerticesPtr = drawTextureWithVertices_d3d11;
	renderer->drawTextureWithVerticesFromIndicesPtr = drawTextureWithVerticesFromIndices_d3d11;

	return SDL_TRUE;

INIT_FAILURE:
	if (swapChain != nullptr && renderer->fullscreen)
	{
		swapChain->SetFullscreenState(false, NULL);
	}

	if (disabledDepthStencilState != nullptr)
	{
		disabledDepthStencilState->Release();
	}

	if (context != nullptr)
	{
		context->Release();
	}

	if (device != nullptr)
	{
		device->Release();
	}

	if (swapChain != nullptr)
	{
		swapChain->Release();
	}

	if (renderer->window != NULL)
	{
		SDL_DestroyWindow(renderer->window);
		renderer->window = NULL;
	}

	return SDL_FALSE;
}

extern "C" void renderFrame_d3d11(Renderer *renderer, void(*drawFunc)(Renderer *))
{
	// Clear back buffer
	ID3D11DeviceContext *context = (ID3D11DeviceContext *)renderer->context;
	ID3D11RenderTargetView *renderTargetView = (ID3D11RenderTargetView *)renderer->renderTargetView;

	float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	context->ClearRenderTargetView(renderTargetView, clearColor);

	// Clear depth buffer
	ID3D11DepthStencilView *depthStencilView = (ID3D11DepthStencilView *)renderer->depthStencilView;
	context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Draw scene
	drawFunc(renderer);

	// Present back buffer to screen
	IDXGISwapChain *swapChain = (IDXGISwapChain *)renderer->swapChain;
	if (renderer->vsync)
	{
		// Lock to screen refresh rate
		swapChain->Present(1, 0);
	}
	else
	{
		// Present as fast as possible
		swapChain->Present(0, 0);
	}
}

extern "C" TextureObject textureFromPixelData_d3d11(Renderer *renderer, const void *pixels, int32_t width, int32_t height)
{
	TextureObject texture;
	texture.glObject = 0;
	return texture;
}

extern "C" void deleteTexture_d3d11(Renderer *renderer, TextureObject texture)
{
}

extern "C" BufferObject createBufferObject_d3d11(Renderer *renderer, const void *data, uint32_t size)
{
	BufferObject buffer;
	buffer.glObject = 0;
	return buffer;
}

extern "C" BufferArrayObject createVertexArrayObject_d3d11(Renderer *renderer, const void *vertices, uint32_t verticesSize)
{
	BufferArrayObject bufferArray;
	bufferArray.glObject = 0;
	return bufferArray;
}

extern "C" BufferArrayObject createVertexAndTextureCoordinateArrayObject_d3d11(Renderer *renderer, const void *verticesAndTextureCoordinates, uint32_t verticesSize, uint32_t textureCoordinatesSize)
{
	BufferArrayObject bufferArray;
	bufferArray.glObject = 0;
	return bufferArray;
}

extern "C" void drawVertices_d3d11(Renderer *renderer, float *modelViewProjectionMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options)
{
	//context->OMSetDepthStencilState(depthStencilState, 1);
}

extern "C" void drawVerticesFromIndices_d3d11(Renderer *renderer, float *modelViewProjectionMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options)
{
}

extern "C" void drawTextureWithVertices_d3d11(Renderer *renderer, float *modelViewProjectionMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options)
{
}

extern "C" void drawTextureWithVerticesFromIndices_d3d11(Renderer *renderer, float *modelViewProjectionMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options)
{
}
