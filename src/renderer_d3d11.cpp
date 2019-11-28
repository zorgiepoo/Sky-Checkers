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
#include "window.h"

#include <stdbool.h>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#define DEPTH_FORMAT DXGI_FORMAT_D24_UNORM_S8_UINT

using namespace DirectX;

extern "C" static void updateViewport_d3d11(Renderer *renderer, int32_t windowWidth, int32_t windowHeight);

extern "C" void renderFrame_d3d11(Renderer *renderer, void(*drawFunc)(Renderer *));

extern "C" TextureObject textureFromPixelData_d3d11(Renderer *renderer, const void *pixels, int32_t width, int32_t height, PixelFormat pixelFormat);

extern "C" void deleteTexture_d3d11(Renderer *renderer, TextureObject texture);

extern "C" BufferObject createIndexBufferObject_d3d11(Renderer *renderer, const void *data, uint32_t size);

extern "C" BufferArrayObject createVertexArrayObject_d3d11(Renderer *renderer, const void *vertices, uint32_t verticesSize);

extern "C" BufferArrayObject createVertexAndTextureCoordinateArrayObject_d3d11(Renderer *renderer, const void *verticesAndTextureCoordinates, uint32_t verticesSize, uint32_t textureCoordinatesSize);

extern "C" void drawVertices_d3d11(Renderer *renderer, float *modelViewProjectionMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options);

extern "C" void drawVerticesFromIndices_d3d11(Renderer *renderer, float *modelViewProjectionMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options);

extern "C" void drawTextureWithVertices_d3d11(Renderer *renderer, float *modelViewProjectionMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options);

extern "C" void drawTextureWithVerticesFromIndices_d3d11(Renderer *renderer, float *modelViewProjectionMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options);

typedef struct
{
	ID3D11ShaderResourceView *resourceView;
	ID3D11Texture2D *texture;
} D3D11TextureDataObject;

extern "C" static void updateViewport_d3d11(Renderer *renderer, int32_t windowWidth, int32_t windowHeight)
{
	renderer->drawableWidth = windowWidth;
	renderer->drawableHeight = windowHeight;

	float aspectRatio = computeProjectionAspectRatio(renderer);
	if (aspectRatio <= 0.001f)
	{
		return;
	}

	IDXGISwapChain *swapChain = (IDXGISwapChain *)renderer->d3d11SwapChain;

	BOOL fullscreenState = false;
	HRESULT fullscreenStateResult = swapChain->GetFullscreenState(&fullscreenState, nullptr);
	if (FAILED(fullscreenStateResult))
	{
		fprintf(stderr, "Error: Failed to retrieve fullscreen state: %d\n", fullscreenStateResult);
		renderer->fullscreen = false;
	}
	else
	{
		renderer->fullscreen = (bool)fullscreenState;
	}

	if (!renderer->fullscreen)
	{
		renderer->windowWidth = windowWidth;
		renderer->windowHeight = windowHeight;
	}

	ID3D11DeviceContext *context = (ID3D11DeviceContext *)renderer->d3d11Context;

	// Release previous render target view and resize swapchain if necessary
	ID3D11RenderTargetView *renderTargetView = (ID3D11RenderTargetView *)renderer->d3d11RenderTargetView;
	if (renderTargetView != nullptr)
	{
		context->OMSetRenderTargets(0, nullptr, nullptr);

		ID3D11Texture2D *depthStencilBuffer = (ID3D11Texture2D *)renderer->d3d11DepthStencilBuffer;
		depthStencilBuffer->Release();

		ID3D11DepthStencilView *depthStencilView = (ID3D11DepthStencilView *)renderer->d3d11DepthStencilView;
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
	D3D11_RENDER_TARGET_VIEW_DESC targetViewDescription;
	ZeroMemory(&targetViewDescription, sizeof(targetViewDescription));
	
	ID3D11Device *device = (ID3D11Device *)renderer->d3d11Device;
	HRESULT targetViewResult = device->CreateRenderTargetView(backBufferPtr, nullptr, &renderTargetView);
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
	depthBufferDescription.Format = DEPTH_FORMAT;
	// Assume no anti-aliasing for now
	depthBufferDescription.SampleDesc.Count = renderer->fsaa ? MSAA_PREFERRED_NONRETINA_SAMPLE_COUNT : 1;
	depthBufferDescription.SampleDesc.Quality = renderer->fsaa ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0;
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

	depthStencilViewDescription.Format = DEPTH_FORMAT;
	depthStencilViewDescription.ViewDimension = renderer->fsaa ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDescription.Texture2D.MipSlice = 0;
	depthStencilViewDescription.Flags = 0;

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
	
	XMMATRIX projectionMatrix = XMMatrixPerspectiveFovRH(PROJECTION_FOV_ANGLE * ((float)M_PI / 180.0f), aspectRatio, PROJECTION_NEAR_VIEW_DISTANCE, PROJECTION_FAR_VIEW_DISTANCE);
	memcpy(renderer->projectionMatrix, &projectionMatrix, sizeof(renderer->projectionMatrix));

	renderer->d3d11RenderTargetView = renderTargetView;
	renderer->d3d11DepthStencilView = depthStencilView;
	renderer->d3d11DepthStencilBuffer = depthStencilBuffer;
}

static bool readFileBytes(const char *filename, void **bytesOutput, SIZE_T *lengthOutput)
{
	FILE *file = fopen(filename, "rb");
	if (file == nullptr)
	{
		fprintf(stderr, "Error: failed to read file: %s\n", filename);
		return false;
	}

	fseek(file, 0, SEEK_END);
	SIZE_T length = (SIZE_T)ftell(file);
	fseek(file, 0, SEEK_SET);

	void *bytes = malloc(length);
	if (bytes == nullptr)
	{
		fprintf(stderr, "Error: failed to allocate bytes for malloc in readFileBytes() for %s\n", filename);
		fclose(file);
		return false;
	}

	if (fread(bytes, length, 1, file) < 1)
	{
		fprintf(stderr, "Error: failed to fread file in readFileBytes() for %s\n", filename);

		free(bytes);
		fclose(file);
		return false;
	}

	fclose(file);

	*bytesOutput = bytes;
	*lengthOutput = length;

	return true;
}

static bool createShader(Renderer *renderer, Shader_d3d11 *shader, const char *vertexShaderName, const char *pixelShaderName, bool textured)
{
	ID3D11Device *device = (ID3D11Device *)renderer->d3d11Device;

	void *vertexShaderBytes;
	SIZE_T vertexShaderLength;
	if (!readFileBytes(vertexShaderName, &vertexShaderBytes, &vertexShaderLength))
	{
		return false;
	}

	ID3D11VertexShader *vertexShader = nullptr;
	HRESULT vertexShaderResult = device->CreateVertexShader(vertexShaderBytes, vertexShaderLength, nullptr, &vertexShader);

	if (FAILED(vertexShaderResult))
	{
		free(vertexShaderBytes);
		fprintf(stderr, "Error: failed to create vertex shader with error %d from %s\n", vertexShaderResult, vertexShaderName);
		return false;
	}

	D3D11_INPUT_ELEMENT_DESC vertexInputLayoutDescription[2];
	ZeroMemory(&vertexInputLayoutDescription, sizeof(vertexInputLayoutDescription));

	vertexInputLayoutDescription[0].SemanticName = "POSITION";
	vertexInputLayoutDescription[0].SemanticIndex = 0;
	vertexInputLayoutDescription[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexInputLayoutDescription[0].InputSlot = 0;
	vertexInputLayoutDescription[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	vertexInputLayoutDescription[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	vertexInputLayoutDescription[0].InstanceDataStepRate = 0;

	if (textured)
	{
		vertexInputLayoutDescription[1].SemanticName = "TEXCOORD";
		vertexInputLayoutDescription[1].SemanticIndex = 0;
		vertexInputLayoutDescription[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		vertexInputLayoutDescription[1].InputSlot = 1;
		vertexInputLayoutDescription[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		vertexInputLayoutDescription[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		vertexInputLayoutDescription[1].InstanceDataStepRate = 0;
	}

	ID3D11InputLayout *vertexInputLayout = nullptr;
	HRESULT createVertexInputLayoutResult = device->CreateInputLayout(vertexInputLayoutDescription, textured ? 2 : 1, vertexShaderBytes, vertexShaderLength, &vertexInputLayout);
	
	free(vertexShaderBytes);

	if (FAILED(createVertexInputLayoutResult))
	{
		fprintf(stderr, "Failed to create vertex input layout with error %d for %s\n", createVertexInputLayoutResult, vertexShaderName);
		vertexShader->Release();
		return false;
	}

	void *pixelShaderBytes;
	SIZE_T pixelShaderLength;
	if (!readFileBytes(pixelShaderName, &pixelShaderBytes, &pixelShaderLength))
	{
		vertexInputLayout->Release();
		vertexShader->Release();
		return false;
	}

	ID3D11PixelShader *pixelShader = nullptr;
	HRESULT pixelShaderResult = device->CreatePixelShader(pixelShaderBytes, pixelShaderLength, nullptr, &pixelShader);

	free(pixelShaderBytes);

	if (FAILED(pixelShaderResult))
	{
		vertexInputLayout->Release();
		vertexShader->Release();
		fprintf(stderr, "Error: failed to create pixel shader with error %d from %s\n", pixelShaderResult, pixelShaderName);
		return false;
	}
	
	shader->vertexShader = vertexShader;
	shader->pixelShader = pixelShader;
	shader->vertexInputLayout = vertexInputLayout;

	return true;
}

static bool createConstantBuffers(Renderer *renderer)
{
	ID3D11Device *device = (ID3D11Device *)renderer->d3d11Device;

	// Set up constant buffer for vertex shader
	D3D11_BUFFER_DESC matrixBufferDescription;
	ZeroMemory(&matrixBufferDescription, sizeof(matrixBufferDescription));

	matrixBufferDescription.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDescription.ByteWidth = sizeof(renderer->projectionMatrix);
	matrixBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDescription.MiscFlags = 0;
	matrixBufferDescription.StructureByteStride = 0;

	ID3D11Buffer *matrixBuffer = nullptr;
	HRESULT createMatrixBufferResult = device->CreateBuffer(&matrixBufferDescription, nullptr, &matrixBuffer);
	if (FAILED(createMatrixBufferResult))
	{
		fprintf(stderr, "Error: Failed to create matrix buffer for shader: %d\n", createMatrixBufferResult);
		return false;
	}

	// Set up constant buffer for pixel shader
	D3D11_BUFFER_DESC colorBufferDescription;
	ZeroMemory(&colorBufferDescription, sizeof(colorBufferDescription));

	colorBufferDescription.Usage = D3D11_USAGE_DYNAMIC;
	colorBufferDescription.ByteWidth = sizeof(float) * 4;
	colorBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	colorBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	colorBufferDescription.MiscFlags = 0;
	colorBufferDescription.StructureByteStride = 0;

	ID3D11Buffer *colorBuffer = nullptr;
	HRESULT createColorBufferResult = device->CreateBuffer(&colorBufferDescription, nullptr, &colorBuffer);
	if (FAILED(createColorBufferResult))
	{
		fprintf(stderr, "Error: Failed to create color buffer for shader: %d\n", createColorBufferResult);
		matrixBuffer->Release();

		return false;
	}

	// Set up texture sampler
	D3D11_SAMPLER_DESC samplerDescription;
	ZeroMemory(&samplerDescription, sizeof(samplerDescription));

	samplerDescription.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDescription.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDescription.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDescription.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDescription.MipLODBias = 0.0f;
	samplerDescription.MaxAnisotropy = 1;
	samplerDescription.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDescription.BorderColor[0] = 0.0f;
	samplerDescription.BorderColor[1] = 0.0f;
	samplerDescription.BorderColor[2] = 0.0f;
	samplerDescription.BorderColor[3] = 0.0f;
	samplerDescription.MinLOD = 0.0f;
	samplerDescription.MaxLOD = D3D11_FLOAT32_MAX;

	ID3D11SamplerState *samplerState = nullptr;
	HRESULT createSamplerResult = device->CreateSamplerState(&samplerDescription, &samplerState);
	if (FAILED(createSamplerResult))
	{
		fprintf(stderr, "Error: Failed to create sampler state: %d\n", createSamplerResult);
		matrixBuffer->Release();
		colorBuffer->Release();

		return false;
	}

	renderer->d3d11VertexShaderConstantBuffer = matrixBuffer;
	renderer->d3d11PixelShaderConstantBuffer = colorBuffer;
	renderer->d3d11SamplerState = samplerState;

	return true;
}

extern "C" bool createRenderer_d3d11(Renderer *renderer, const char *windowTitle, int32_t windowWidth, int32_t windowHeight, bool fullscreen, bool vsync, bool fsaa)
{
	// Need to initialize D3D states here in order to goto INIT_FAILURE on failure
	ID3D11Device *device = nullptr;
	IDXGISwapChain *swapChain = nullptr;
	ID3D11DeviceContext *context = nullptr;
	ID3D11DepthStencilState *depthStencilState = nullptr;
	ID3D11DepthStencilState *disabledDepthStencilState = nullptr;
	ID3D11BlendState *alphaBlendState = nullptr;
	ID3D11BlendState *oneMinusAlphaBlendState = nullptr;
	ID3D11RasterizerState *rasterState = nullptr;

	renderer->windowWidth = max(windowWidth, 1);
	renderer->windowHeight = max(windowHeight, 1);

	renderer->window = renderer->window = ZGCreateWindow(windowTitle, windowWidth, windowHeight, nullptr);
	if (renderer->window == NULL)
	{
		goto INIT_FAILURE;
	}

	ZGSetWindowMinimumSize(renderer->window, 8, 8);
	ZGGetWindowSize(renderer->window, &renderer->windowWidth, &renderer->windowHeight);

	renderer->fullscreen = fullscreen;

	// Initialize with default adapter
	IDXGIAdapter *adapter = nullptr;
	
	// Find the best adapter possible
	IDXGIFactory1 *factoryForAdapter = nullptr;
	HRESULT createFactoryResult = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void **)&factoryForAdapter);
	if (FAILED(createFactoryResult))
	{
		factoryForAdapter = nullptr;
	}
	else
	{
		IDXGIAdapter1 *currentAdapter = nullptr;
		UINT enumeratingAdapterIndex = 0;
		UINT adapterInsertionIndex = 0;
		INT maxScoreIndex = -1;
		
		const UINT maxAdaptersCount = 256;
		IDXGIAdapter1 *adapters[maxAdaptersCount];
		uint64_t adapterScores[maxAdaptersCount];

		while (factoryForAdapter->EnumAdapters1(enumeratingAdapterIndex, &currentAdapter) != DXGI_ERROR_NOT_FOUND)
		{
			DXGI_ADAPTER_DESC1 adapter1Desc;
			HRESULT getAdapter1DescResult = currentAdapter->GetDesc1(&adapter1Desc);
			if (!FAILED(getAdapter1DescResult))
			{
				adapters[adapterInsertionIndex] = currentAdapter;

				uint64_t hardwareScore = ((adapter1Desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != DXGI_ADAPTER_FLAG_SOFTWARE) ? 10000000000000 : 0;
				uint64_t score = hardwareScore + adapter1Desc.DedicatedSystemMemory + adapter1Desc.DedicatedVideoMemory + adapter1Desc.SharedSystemMemory;

				adapterScores[adapterInsertionIndex] = score;

				if (maxScoreIndex < 0 || score > adapterScores[maxScoreIndex])
				{
					maxScoreIndex = (INT)adapterInsertionIndex;
				}

				adapterInsertionIndex++;

				if (adapterInsertionIndex >= maxAdaptersCount)
				{
					break;
				}
			}
			else
			{
				currentAdapter->Release();
			}

			enumeratingAdapterIndex++;
		}

		for (UINT adapterIndex = 0; adapterIndex < adapterInsertionIndex; adapterIndex++)
		{
			if (maxScoreIndex == (INT)adapterIndex)
			{
				adapter = adapters[adapterIndex];
			}
			else
			{
				adapters[adapterIndex]->Release();
			}
		}
	}

	if (factoryForAdapter != nullptr)
	{
		factoryForAdapter->Release();
		factoryForAdapter = nullptr;
	}

	UINT deviceFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#ifdef _DEBUG
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	
	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };

	HRESULT deviceResult = D3D11CreateDevice(adapter, (adapter != nullptr ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE), nullptr, deviceFlags, featureLevels, sizeof(featureLevels) / sizeof(*featureLevels), D3D11_SDK_VERSION, &device, nullptr, &context);
	
	if (adapter != nullptr)
	{
		adapter->Release();
		adapter = nullptr;
	}

	if (FAILED(deviceResult))
	{
		fprintf(stderr, "Error: Failed to create D3D11 device: %d\n", deviceResult);
		device = nullptr;
		context = nullptr;

		goto INIT_FAILURE;
	}
	
	// Determine anti aliasing
	if (fsaa)
	{
		UINT numberOfQualityLevels = 0;
		HRESULT multisampleResult = device->CheckMultisampleQualityLevels(DEPTH_FORMAT, MSAA_PREFERRED_NONRETINA_SAMPLE_COUNT, &numberOfQualityLevels);
		if (FAILED(multisampleResult))
		{
			fprintf(stderr, "Error: failed to check multisample quality levels\n");
			renderer->fsaa = false;
		}
		else
		{
			renderer->fsaa = (bool)(numberOfQualityLevels > 0);
		}
	}
	else
	{
		renderer->fsaa = false;
	}

	renderer->vsync = vsync;

	// Retrieve factory from device
	// See https://docs.microsoft.com/en-us/windows/desktop/api/dxgi/nn-dxgi-idxgifactory remarks
	IDXGIDevice *pDXGIDevice = nullptr;
	HRESULT queryInterfaceResult = device->QueryInterface(__uuidof(IDXGIDevice), (void **)&pDXGIDevice);
	if (FAILED(queryInterfaceResult))
	{
		fprintf(stderr, "Error: Failed to query interface from device: %d\n", queryInterfaceResult);
		goto INIT_FAILURE;
	}

	IDXGIAdapter *pDXGIAdapter = nullptr;
	HRESULT getAdapterResult = pDXGIDevice->GetAdapter(&pDXGIAdapter);
	if (FAILED(getAdapterResult))
	{
		fprintf(stderr, "Error: Failed to get adapter from device: %d\n", getAdapterResult);
		goto INIT_FAILURE;
	}

	IDXGIFactory *pIDXGIFactory = nullptr;
	HRESULT getParentResult = pDXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void **)&pIDXGIFactory);
	if (FAILED(getParentResult))
	{
		fprintf(stderr, "Error: Failed to get parent from device: %d\n", getParentResult);
		goto INIT_FAILURE;
	}

	// Create swapchain
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	// Infer window width/height from our window
	swapChainDesc.BufferDesc.Width = 0;
	swapChainDesc.BufferDesc.Height = 0;

	// Leaving RefreshRate to 0 in swap chain seems to be fine
	// No need to query for display's refresh rate
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;

	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	swapChainDesc.SampleDesc.Count = renderer->fsaa ? MSAA_PREFERRED_NONRETINA_SAMPLE_COUNT : 1;
	swapChainDesc.SampleDesc.Quality = renderer->fsaa ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0;
	
	swapChainDesc.BufferCount = 1;
	swapChainDesc.Windowed = !renderer->fullscreen;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = 0;
	
	swapChainDesc.OutputWindow = (HWND)ZGWindowHandle(renderer->window);

	HRESULT swapChainResult = pIDXGIFactory->CreateSwapChain(device, &swapChainDesc, &swapChain);
	if (FAILED(swapChainResult))
	{
		fprintf(stderr, "Error: Failed to create D3D11 swap chain: %d\n", swapChainResult);
		swapChain = nullptr;
		goto INIT_FAILURE;
	}

	// Create depth stencil state
	D3D11_DEPTH_STENCIL_DESC depthStencilDescription;
	ZeroMemory(&depthStencilDescription, sizeof(depthStencilDescription));

	depthStencilDescription.DepthEnable = TRUE;
	depthStencilDescription.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDescription.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	depthStencilDescription.StencilEnable = FALSE;
	depthStencilDescription.StencilReadMask = 0xFF;
	depthStencilDescription.StencilWriteMask = 0xFF;
	depthStencilDescription.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDescription.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDescription.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDescription.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	depthStencilDescription.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDescription.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDescription.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDescription.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	HRESULT depthStencilResult = device->CreateDepthStencilState(&depthStencilDescription, &depthStencilState);
	if (FAILED(depthStencilResult))
	{
		fprintf(stderr, "Error: Failed to create D3D11 depth stencil: %d\n", depthStencilResult);
		depthStencilState = nullptr;
		goto INIT_FAILURE;
	}

	// Create disabled depth stencil state
	D3D11_DEPTH_STENCIL_DESC disabledDepthStencilDescription;
	ZeroMemory(&disabledDepthStencilDescription, sizeof(disabledDepthStencilDescription));

	disabledDepthStencilDescription.DepthEnable = FALSE;
	disabledDepthStencilDescription.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	disabledDepthStencilDescription.DepthFunc = D3D11_COMPARISON_LESS;
	disabledDepthStencilDescription.StencilEnable = FALSE;
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
	
	HRESULT disabledDepthStencilResult = device->CreateDepthStencilState(&disabledDepthStencilDescription, &disabledDepthStencilState);
	if (FAILED(disabledDepthStencilResult))
	{
		fprintf(stderr, "Error: Failed to create D3D11 depth stencil: %d\n", disabledDepthStencilResult);
		disabledDepthStencilState = nullptr;
		goto INIT_FAILURE;
	}

	// Create alpha blend state
	D3D11_BLEND_DESC alphaBlendDescription;
	ZeroMemory(&alphaBlendDescription, sizeof(alphaBlendDescription));

	alphaBlendDescription.AlphaToCoverageEnable = FALSE;
	alphaBlendDescription.IndependentBlendEnable = FALSE;
	alphaBlendDescription.RenderTarget[0].BlendEnable = TRUE;
	alphaBlendDescription.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	alphaBlendDescription.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_ALPHA;
	alphaBlendDescription.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	alphaBlendDescription.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	alphaBlendDescription.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	alphaBlendDescription.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	alphaBlendDescription.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	HRESULT createAlphaBlendResult = device->CreateBlendState(&alphaBlendDescription, &alphaBlendState);
	if (FAILED(createAlphaBlendResult))
	{
		fprintf(stderr, "Error: Failed to create D3D11 alpha blend state: %d\n", createAlphaBlendResult);
		alphaBlendState = nullptr;
		goto INIT_FAILURE;
	}

	// Create one minus alpha blend state
	D3D11_BLEND_DESC oneMinusAlphaBlendDescription;
	ZeroMemory(&oneMinusAlphaBlendDescription, sizeof(oneMinusAlphaBlendDescription));

	oneMinusAlphaBlendDescription.AlphaToCoverageEnable = FALSE;
	oneMinusAlphaBlendDescription.IndependentBlendEnable = FALSE;
	oneMinusAlphaBlendDescription.RenderTarget[0].BlendEnable = TRUE;
	oneMinusAlphaBlendDescription.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	oneMinusAlphaBlendDescription.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	oneMinusAlphaBlendDescription.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	oneMinusAlphaBlendDescription.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	oneMinusAlphaBlendDescription.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	oneMinusAlphaBlendDescription.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	oneMinusAlphaBlendDescription.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	HRESULT createOneMinusAlphaBlendResult = device->CreateBlendState(&oneMinusAlphaBlendDescription, &oneMinusAlphaBlendState);
	if (FAILED(createOneMinusAlphaBlendResult))
	{
		fprintf(stderr, "Error: Failed to create D3D11 one minus alpha blend state: %d\n", createOneMinusAlphaBlendResult);
		oneMinusAlphaBlendState = nullptr;
		goto INIT_FAILURE;
	}

	// Set up raster description
	D3D11_RASTERIZER_DESC rasterDescription;
	ZeroMemory(&rasterDescription, sizeof(rasterDescription));

	rasterDescription.AntialiasedLineEnable = renderer->fsaa;
	rasterDescription.CullMode = D3D11_CULL_NONE;
	rasterDescription.DepthBias = 0;
	rasterDescription.DepthBiasClamp = 0.0f;
	rasterDescription.DepthClipEnable = TRUE;
	rasterDescription.FillMode = D3D11_FILL_SOLID;
	rasterDescription.FrontCounterClockwise = FALSE;
	rasterDescription.MultisampleEnable = renderer->fsaa;
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

	renderer->d3d11Device = device;

	if (!createConstantBuffers(renderer))
	{
		fprintf(stderr, "Error: Failed to create constant buffers\n");
		goto INIT_FAILURE;
	}

	if (!createShader(renderer, &renderer->d3d11PositionShader, "Data\\Shaders\\position-vertex.cso", "Data\\Shaders\\position-pixel.cso", false))
	{
		fprintf(stderr, "Error: Failed to create position shader\n");
		goto INIT_FAILURE;
	}

	if (!createShader(renderer, &renderer->d3d11TexturePositionShader, "Data\\Shaders\\texture-position-vertex.cso", "Data\\Shaders\\texture-position-pixel.cso", true))
	{
		fprintf(stderr, "Error: Failed to create texture position shader\n");
		goto INIT_FAILURE;
	}

	renderer->d3d11Context = context;
	renderer->d3d11SwapChain = swapChain;

	renderer->d3d11RenderTargetView = nullptr;
	renderer->d3d11DepthStencilView = nullptr;
	renderer->d3d11DepthStencilBuffer = nullptr;
	updateViewport_d3d11(renderer, renderer->windowWidth, renderer->windowHeight);

	renderer->d3d11DepthStencilState = depthStencilState;
	renderer->d3d11DisabledDepthStencilState = disabledDepthStencilState;

	renderer->d3d11AlphaBlendState = alphaBlendState;
	renderer->d3d11OneMinusAlphaBlendState = oneMinusAlphaBlendState;

	renderer->updateViewportPtr = updateViewport_d3d11;
	renderer->renderFramePtr = renderFrame_d3d11;
	renderer->textureFromPixelDataPtr = textureFromPixelData_d3d11;
	renderer->deleteTexturePtr = deleteTexture_d3d11;
	renderer->createIndexBufferObjectPtr = createIndexBufferObject_d3d11;
	renderer->createVertexArrayObjectPtr = createVertexArrayObject_d3d11;
	renderer->createVertexAndTextureCoordinateArrayObjectPtr = createVertexAndTextureCoordinateArrayObject_d3d11;
	renderer->drawVerticesPtr = drawVertices_d3d11;
	renderer->drawVerticesFromIndicesPtr = drawVerticesFromIndices_d3d11;
	renderer->drawTextureWithVerticesPtr = drawTextureWithVertices_d3d11;
	renderer->drawTextureWithVerticesFromIndicesPtr = drawTextureWithVerticesFromIndices_d3d11;

	return true;

INIT_FAILURE:
	if (swapChain != nullptr && renderer->fullscreen)
	{
		swapChain->SetFullscreenState(false, NULL);
	}

	if (depthStencilState != nullptr)
	{
		depthStencilState->Release();
	}

	if (disabledDepthStencilState != nullptr)
	{
		disabledDepthStencilState->Release();
	}

	if (alphaBlendState != nullptr)
	{
		alphaBlendState->Release();
	}

	if (oneMinusAlphaBlendState != nullptr)
	{
		oneMinusAlphaBlendState->Release();
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
		ZGDestroyWindow(renderer->window);
		renderer->window = NULL;
	}

	return false;
}

extern "C" void renderFrame_d3d11(Renderer *renderer, void(*drawFunc)(Renderer *))
{
	// Clear back buffer
	ID3D11DeviceContext *context = (ID3D11DeviceContext *)renderer->d3d11Context;
	ID3D11RenderTargetView *renderTargetView = (ID3D11RenderTargetView *)renderer->d3d11RenderTargetView;

	float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	context->ClearRenderTargetView(renderTargetView, clearColor);

	// Clear depth buffer
	ID3D11DepthStencilView *depthStencilView = (ID3D11DepthStencilView *)renderer->d3d11DepthStencilView;
	context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Draw scene
	drawFunc(renderer);

	// Present back buffer to screen
	IDXGISwapChain *swapChain = (IDXGISwapChain *)renderer->d3d11SwapChain;
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

extern "C" TextureObject textureFromPixelData_d3d11(Renderer *renderer, const void *pixels, int32_t width, int32_t height, PixelFormat pixelFormat)
{
	D3D11_SUBRESOURCE_DATA textureData;
	ZeroMemory(&textureData, sizeof(textureData));

	const UINT bytesPerRow = 4;

	textureData.pSysMem = pixels;
	textureData.SysMemPitch = bytesPerRow * (UINT)width;
	textureData.SysMemSlicePitch = 0;

	D3D11_TEXTURE2D_DESC textureDescription;
	ZeroMemory(&textureDescription, sizeof(textureDescription));

	DXGI_FORMAT format;
	switch (pixelFormat)
	{
	case PIXEL_FORMAT_RGBA32:
		format = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case PIXEL_FORMAT_BGRA32:
		format = DXGI_FORMAT_B8G8R8A8_UNORM;
		break;
	}

	textureDescription.Width = width;
	textureDescription.Height = height;
	textureDescription.MipLevels = 1;
	textureDescription.ArraySize = 1;
	textureDescription.Format = format;
	textureDescription.SampleDesc.Count = 1;
	textureDescription.SampleDesc.Quality = 0;
	textureDescription.Usage = D3D11_USAGE_DEFAULT;
	textureDescription.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDescription.CPUAccessFlags = 0;
	textureDescription.MiscFlags = 0;

	ID3D11Device *device = (ID3D11Device *)renderer->d3d11Device;
	ID3D11Texture2D *texture = nullptr;
	HRESULT textureResult = device->CreateTexture2D(&textureDescription, &textureData, &texture);
	if (FAILED(textureResult))
	{
		fprintf(stderr, "Failed to create d3d texture: %d\n", textureResult);
		abort();
	}

	ID3D11ShaderResourceView *resourceView = nullptr;
	HRESULT resourceViewResult = device->CreateShaderResourceView(texture, nullptr, &resourceView);
	if (FAILED(resourceViewResult))
	{
		fprintf(stderr, "Failed to create d3d texture resource view: %d\n", resourceViewResult);
		abort();
	}

	D3D11TextureDataObject *textureDataObject = (D3D11TextureDataObject *)malloc(sizeof(*textureDataObject));
	textureDataObject->resourceView = resourceView;
	textureDataObject->texture = texture;

	TextureObject textureObject;
	textureObject.d3d11Object = textureDataObject;
	return textureObject;
}

extern "C" void deleteTexture_d3d11(Renderer *renderer, TextureObject textureObject)
{
	D3D11TextureDataObject *textureDataObject = (D3D11TextureDataObject *)textureObject.d3d11Object;

	ID3D11ShaderResourceView *textureResourceView = textureDataObject->resourceView;
	textureResourceView->Release();

	ID3D11Texture2D *texture = textureDataObject->texture;
	texture->Release();

	free(textureDataObject);
}

static ID3D11Buffer *createVertexBuffer(ID3D11Device *device, const void *data, uint32_t size, D3D11_BIND_FLAG bindFlags)
{
	D3D11_BUFFER_DESC vertexBufferDescription;
	ZeroMemory(&vertexBufferDescription, sizeof(vertexBufferDescription));

	vertexBufferDescription.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDescription.ByteWidth = size;
	vertexBufferDescription.BindFlags = bindFlags;
	vertexBufferDescription.CPUAccessFlags = 0;
	vertexBufferDescription.MiscFlags = 0;
	vertexBufferDescription.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexData;
	ZeroMemory(&vertexData, sizeof(vertexData));

	vertexData.pSysMem = data;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	ID3D11Buffer *vertexBuffer = nullptr;
	HRESULT createBufferResult = device->CreateBuffer(&vertexBufferDescription, &vertexData, &vertexBuffer);
	
	if (FAILED(createBufferResult))
	{
		fprintf(stderr, "Failed to create vertex buffer with error: %d\n", createBufferResult);
		abort();
	}

	return vertexBuffer;
}

extern "C" BufferObject createIndexBufferObject_d3d11(Renderer *renderer, const void *data, uint32_t size)
{
	BufferObject buffer;
	buffer.d3d11Object = createVertexBuffer((ID3D11Device *)renderer->d3d11Device, data, size, D3D11_BIND_INDEX_BUFFER);
	return buffer;
}

extern "C" BufferArrayObject createVertexArrayObject_d3d11(Renderer *renderer, const void *vertices, uint32_t verticesSize)
{
	BufferArrayObject bufferArray;
	bufferArray.d3d11Object = createVertexBuffer((ID3D11Device *)renderer->d3d11Device, vertices, verticesSize, D3D11_BIND_VERTEX_BUFFER);
	bufferArray.d3d11VerticesSize = verticesSize;
	return bufferArray;
}

extern "C" BufferArrayObject createVertexAndTextureCoordinateArrayObject_d3d11(Renderer *renderer, const void *verticesAndTextureCoordinates, uint32_t verticesSize, uint32_t textureCoordinatesSize)
{
	BufferArrayObject bufferArray;
	bufferArray.d3d11Object = createVertexBuffer((ID3D11Device *)renderer->d3d11Device, verticesAndTextureCoordinates, verticesSize + textureCoordinatesSize, D3D11_BIND_VERTEX_BUFFER);
	bufferArray.d3d11VerticesSize = verticesSize;
	return bufferArray;
}

static D3D11_PRIMITIVE_TOPOLOGY primitiveTopologyFromRendererMode(RendererMode mode)
{
	switch (mode)
	{
	case RENDERER_TRIANGLE_MODE:
		return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case RENDERER_TRIANGLE_STRIP_MODE:
		return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	}
	return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

static void copyDataToDynamicBuffer(ID3D11DeviceContext *context, ID3D11Buffer *buffer, void *data, size_t size)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	HRESULT mappedResult = context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
#ifdef _DEBUG
	if (FAILED(mappedResult))
	{
		fprintf(stderr, "Error: failed to Map() dynamic buffer: %d\n", mappedResult);
		abort();
	}
#endif

	memcpy(mappedResource.pData, data, size);

	context->Unmap(buffer, 0);
}

static void encodeRendererAndShaderState(Renderer *renderer, Shader_d3d11 *shader, float *modelViewProjectionMatrix, RendererMode mode, color4_t color, RendererOptions options)
{
	ID3D11DeviceContext *context = (ID3D11DeviceContext *)renderer->d3d11Context;
	
	if ((options & RENDERER_OPTION_DISABLE_DEPTH_TEST) != 0)
	{
		ID3D11DepthStencilState *disabledDepthStencilState = (ID3D11DepthStencilState *)renderer->d3d11DisabledDepthStencilState;
		context->OMSetDepthStencilState(disabledDepthStencilState, 0);
	}
	else
	{
		ID3D11DepthStencilState *depthStencilState = (ID3D11DepthStencilState *)renderer->d3d11DepthStencilState;
		context->OMSetDepthStencilState(depthStencilState, 0);
	}

	if ((options & RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA) != 0)
	{
		ID3D11BlendState *oneMinusAlphaBlendState = (ID3D11BlendState *)renderer->d3d11OneMinusAlphaBlendState;
		context->OMSetBlendState(oneMinusAlphaBlendState, nullptr, 0xffffffff);
	}
	else if ((options & RENDERER_OPTION_BLENDING_ALPHA) != 0)
	{
		ID3D11BlendState *alphaBlendState = (ID3D11BlendState *)renderer->d3d11AlphaBlendState;
		context->OMSetBlendState(alphaBlendState, nullptr, 0xffffffff);
	}
	else
	{
		context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	}

	context->IASetPrimitiveTopology(primitiveTopologyFromRendererMode(mode));

	ID3D11Buffer *vertexConstantBuffer = (ID3D11Buffer *)renderer->d3d11VertexShaderConstantBuffer;
	copyDataToDynamicBuffer(context, vertexConstantBuffer, modelViewProjectionMatrix, sizeof(renderer->projectionMatrix));
	context->VSSetConstantBuffers(0, 1, &vertexConstantBuffer);

	ID3D11Buffer *pixelConstantBuffer = (ID3D11Buffer *)renderer->d3d11PixelShaderConstantBuffer;
	copyDataToDynamicBuffer(context, pixelConstantBuffer, &color, sizeof(color));
	context->PSSetConstantBuffers(0, 1, &pixelConstantBuffer);

	ID3D11InputLayout *vertexInputLayout = (ID3D11InputLayout *)shader->vertexInputLayout;
	context->IASetInputLayout(vertexInputLayout);

	ID3D11VertexShader *vertexShader = (ID3D11VertexShader *)shader->vertexShader;
	context->VSSetShader(vertexShader, nullptr, 0);

	ID3D11PixelShader *pixelShader = (ID3D11PixelShader *)shader->pixelShader;
	context->PSSetShader(pixelShader, nullptr, 0);
}

static void encodeVertices(Renderer *renderer, BufferArrayObject vertexArrayObject)
{
	ID3D11DeviceContext *context = (ID3D11DeviceContext *)renderer->d3d11Context;
	
	ID3D11Buffer *vertexBuffer = (ID3D11Buffer *)vertexArrayObject.d3d11Object;
	UINT stride = sizeof(float) * 3;
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
}

static void encodeVerticesAndTextureCoordinates(Renderer *renderer, BufferArrayObject vertexAndTextureArrayObject)
{
	ID3D11DeviceContext *context = (ID3D11DeviceContext *)renderer->d3d11Context;

	ID3D11Buffer *vertexAndTextureCoordinateBuffer = (ID3D11Buffer *)vertexAndTextureArrayObject.d3d11Object;
	ID3D11Buffer *bufferArray[] = { vertexAndTextureCoordinateBuffer , vertexAndTextureCoordinateBuffer };
	UINT strides[] = { sizeof(float) * 3, sizeof(float) * 2 };
	UINT offsets[] = { 0, vertexAndTextureArrayObject.d3d11VerticesSize };

	context->IASetVertexBuffers(0, 2, bufferArray, strides, offsets);
}

static void encodeTexture(Renderer *renderer, TextureObject texture)
{
	ID3D11DeviceContext *context = (ID3D11DeviceContext *)renderer->d3d11Context;

	D3D11TextureDataObject *textureDataObject = (D3D11TextureDataObject *)texture.d3d11Object;
	ID3D11ShaderResourceView *resourceView = textureDataObject->resourceView;
	context->PSSetShaderResources(0, 1, &resourceView);

	ID3D11SamplerState *samplerState = (ID3D11SamplerState *)renderer->d3d11SamplerState;
	context->PSSetSamplers(0, 1, &samplerState);
}

static void encodeIndexBuffer(Renderer *renderer, BufferObject indicesBufferObject)
{
	ID3D11DeviceContext *context = (ID3D11DeviceContext *)renderer->d3d11Context;

	ID3D11Buffer *indexBuffer = (ID3D11Buffer *)indicesBufferObject.d3d11Object;
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
}

extern "C" void drawVertices_d3d11(Renderer *renderer, float *modelViewProjectionMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options)
{
	encodeRendererAndShaderState(renderer, &renderer->d3d11PositionShader, modelViewProjectionMatrix, mode, color, options);
	encodeVertices(renderer, vertexArrayObject);

	ID3D11DeviceContext *context = (ID3D11DeviceContext *)renderer->d3d11Context;
	context->Draw(vertexCount, 0);
}

extern "C" void drawVerticesFromIndices_d3d11(Renderer *renderer, float *modelViewProjectionMatrix, RendererMode mode, BufferArrayObject vertexArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options)
{
	encodeRendererAndShaderState(renderer, &renderer->d3d11PositionShader, modelViewProjectionMatrix, mode, color, options);
	encodeVertices(renderer, vertexArrayObject);
	encodeIndexBuffer(renderer, indicesBufferObject);

	ID3D11DeviceContext *context = (ID3D11DeviceContext *)renderer->d3d11Context;
	context->DrawIndexed(indicesCount, 0, 0);
}

extern "C" void drawTextureWithVertices_d3d11(Renderer *renderer, float *modelViewProjectionMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, uint32_t vertexCount, color4_t color, RendererOptions options)
{
	ID3D11DeviceContext *context = (ID3D11DeviceContext *)renderer->d3d11Context;

	encodeRendererAndShaderState(renderer, &renderer->d3d11TexturePositionShader, modelViewProjectionMatrix, mode, color, options);
	encodeVerticesAndTextureCoordinates(renderer, vertexAndTextureArrayObject);
	encodeTexture(renderer, texture);

	context->Draw(vertexCount, 0);
}

extern "C" void drawTextureWithVerticesFromIndices_d3d11(Renderer *renderer, float *modelViewProjectionMatrix, TextureObject texture, RendererMode mode, BufferArrayObject vertexAndTextureArrayObject, BufferObject indicesBufferObject, uint32_t indicesCount, color4_t color, RendererOptions options)
{
	encodeRendererAndShaderState(renderer, &renderer->d3d11TexturePositionShader, modelViewProjectionMatrix, mode, color, options);
	encodeVerticesAndTextureCoordinates(renderer, vertexAndTextureArrayObject);
	encodeTexture(renderer, texture);
	encodeIndexBuffer(renderer, indicesBufferObject);

	ID3D11DeviceContext *context = (ID3D11DeviceContext *)renderer->d3d11Context;
	context->DrawIndexed(indicesCount, 0, 0);
}
