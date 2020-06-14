/*
 * Copyright 2020 Mayur Pawashe
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
