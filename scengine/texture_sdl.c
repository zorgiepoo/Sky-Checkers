/*
 MIT License

 Copyright (c) 2010 Mayur Pawashe

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

#include <SDL2/SDL.h>

#include "texture.h"
#include "quit.h"

static void *create8BitPixelDataWithAlpha(SDL_Surface *surface)
{
	uint8_t bytesPerPixel = surface->format->BytesPerPixel;
	const uint8_t newBytesPerPixel = 4;
	
	uint8_t *pixelData = calloc(surface->h, newBytesPerPixel * sizeof(uint8_t) * surface->w);
	if (pixelData == NULL)
	{
		fprintf(stderr, "Error: failed to allocate pixel data\n");
		ZGQuit();
	}
	
	uint8_t *surfacePixelData = surface->pixels;
	for (uint32_t pixelIndex = 0; pixelIndex < (uint32_t)(surface->w * surface->h); pixelIndex++)
	{
		memcpy(pixelData + newBytesPerPixel * pixelIndex, surfacePixelData + bytesPerPixel * pixelIndex, bytesPerPixel);
		pixelData[newBytesPerPixel * pixelIndex + (newBytesPerPixel - 1)] = 0xFF;
	}
	
	return pixelData;
}

TextureData loadTextureData(const char *filePath)
{
	SDL_Surface *surface = SDL_LoadBMP(filePath);
	if (surface == NULL)
	{
		fprintf(stderr, "Couldn't load texture: %s\n", filePath);
		ZGQuit();
	}

	void *pixelData = NULL;
	bool sourceMissingAlpha = (surface->format->BytesPerPixel == 3);
	if (sourceMissingAlpha)
	{
		pixelData = create8BitPixelDataWithAlpha(surface);
	}
	else
	{
		pixelData = surface->pixels;
	}

	TextureData textureData = {.pixelData = pixelData, .width = surface->w, .height = surface->h, .pixelFormat = PIXEL_FORMAT_BGRA32};
	if (sourceMissingAlpha)
	{
		SDL_FreeSurface(surface);
	}
	else
	{
		textureData.context = surface;
	}

	return textureData;
}

void freeTextureData(TextureData textureData)
{
	if (textureData.context != NULL)
	{
		SDL_FreeSurface(textureData.context);
	}
	else
	{
		free(textureData.pixelData);
	}
}
