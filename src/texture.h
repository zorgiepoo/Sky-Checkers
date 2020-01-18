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

#pragma once

#include "renderer.h"
#include "platforms.h"

typedef struct
{
	uint8_t *pixelData;
#if !PLATFORM_APPLE
	void *surface;
#endif
	int32_t width;
	int32_t height;
	// Pixel format is guaranteed to have 4 8-bit components including alpha component at the end
	PixelFormat pixelFormat;
} TextureData;

TextureData loadTextureData(const char *filePath);
void freeTextureData(TextureData textureData);

TextureObject loadTextureFromData(Renderer *renderer, TextureData textureData);
TextureObject loadTexture(Renderer *renderer, const char *filePath);
