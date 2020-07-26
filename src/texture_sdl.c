/*
 * Copyright 2010 Mayur Pawashe
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
