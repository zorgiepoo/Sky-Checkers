/*
 MIT License

 Copyright (c) 2024 Mayur Pawashe

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

#include "texture.h"

#include <stdio.h>
#include <stdlib.h>
#include <wincodec.h>
#include <wincodecsdk.h>

extern "C" TextureData loadTextureData(const char* filePath)
{
	static IWICImagingFactory* imagingFactory;
	if (imagingFactory == nullptr)
	{
		HRESULT createFactoryResult = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&imagingFactory));
		if (FAILED(createFactoryResult))
		{
			fprintf(stderr, "Error: failed to create WIC imaging factory: %d\n", createFactoryResult);
			abort();
		}
	}
	
	size_t filePathLength = strlen(filePath);
	wchar_t *wideFilePath = (wchar_t *)calloc(sizeof(*wideFilePath), filePathLength + 1);
	for (size_t filePathIndex = 0; filePathIndex < filePathLength; filePathIndex++)
	{
		wideFilePath[filePathIndex] = filePath[filePathIndex];
	}

	IWICBitmapDecoder* decoder = nullptr;
	HRESULT decoderResult = imagingFactory->CreateDecoderFromFilename(wideFilePath, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
	if (FAILED(decoderResult))
	{
		fprintf(stderr, "Error: failed to create decoder for %s : %d\n", filePath, decoderResult);
		abort();
	}

	free(wideFilePath);

	IWICBitmapFrameDecode* bitmapFrameDecode = nullptr;
	HRESULT getFrameResult = decoder->GetFrame(0, &bitmapFrameDecode);
	if (FAILED(getFrameResult))
	{
		fprintf(stderr, "Error: failed to get frame for %s : %d\n", filePath, getFrameResult);
		abort();
	}

	IWICFormatConverter* formatConverter = nullptr;
	HRESULT formatConverterResult = imagingFactory->CreateFormatConverter(&formatConverter);
	if (FAILED(formatConverterResult))
	{
		fprintf(stderr, "Error: failed to create format converter for %s : %d\n", filePath, formatConverterResult);
		abort();
	}

	const UINT bytesPerPixel = 4;
	HRESULT initConverterResult = formatConverter->Initialize(bitmapFrameDecode, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
	if (FAILED(initConverterResult))
	{
		fprintf(stderr, "Error: failed to create format converter for %s : %d\n", filePath, initConverterResult);
		abort();
	}
	
	UINT width = 0;
	UINT height = 0;
	HRESULT sizeResult = formatConverter->GetSize(&width, &height);
	if (FAILED(sizeResult))
	{
		fprintf(stderr, "Error: failed to get bitmap size for %s : %d\n", filePath, sizeResult);
		abort();
	}

	const UINT dataSize = width * height * bytesPerPixel;
	BYTE* pixelData = (BYTE *)calloc(1, dataSize);
	
	HRESULT copyPixelsResult = formatConverter->CopyPixels(nullptr, width * bytesPerPixel, dataSize, pixelData);
	if (FAILED(copyPixelsResult))
	{
		fprintf(stderr, "Error: failed to copy pixels for %s : %d\n", filePath, copyPixelsResult);
		abort();
	}

	formatConverter->Release();
	bitmapFrameDecode->Release();
	decoder->Release();

	TextureData textureData = { 0 };
	textureData.pixelData = pixelData;
	textureData.width = (int32_t)width;
	textureData.height = (int32_t)height;
	textureData.pixelFormat = PIXEL_FORMAT_RGBA32;
	
	return textureData;
}

extern "C" void freeTextureData(TextureData textureData)
{
	free(textureData.pixelData);
}
