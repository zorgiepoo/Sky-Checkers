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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "renderer_types.h"
#include "platforms.h"

#define TEXTURE_DATA_HAS_CONTEXT !PLATFORM_WINDOWS

typedef struct
{
	uint8_t* pixelData;
#if TEXTURE_DATA_HAS_CONTEXT
	void* context;
#endif
	int32_t width;
	int32_t height;
	// Pixel format is guaranteed to have 4 8-bit components including alpha component at the end
	PixelFormat pixelFormat;
} TextureData;

TextureData loadTextureData(const char* filePath);
TextureData copyTextureData(TextureData textureData);
void freeTextureData(TextureData textureData);

TextureObject loadTextureFromData(Renderer* renderer, TextureData textureData);
TextureObject loadTexture(Renderer* renderer, const char* filePath);

#ifdef __cplusplus
}
#endif
