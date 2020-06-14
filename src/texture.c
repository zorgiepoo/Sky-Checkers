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

#include "texture.h"
#include "renderer.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

TextureObject loadTextureFromData(Renderer *renderer, TextureData textureData)
{
	return textureFromPixelData(renderer, textureData.pixelData, textureData.width, textureData.height, textureData.pixelFormat);
}

TextureObject loadTexture(Renderer *renderer, const char *filePath)
{
	TextureData textureData = loadTextureData(filePath);
	
	TextureObject texture = loadTextureFromData(renderer, textureData);
	
	freeTextureData(textureData);
	
	return texture;
}

TextureData copyTextureData(TextureData textureData)
{
	TextureData copyData = textureData;
#if TEXTURE_DATA_HAS_CONTEXT
	copyData.context = NULL;
#endif
	
	size_t numBytes = textureData.width * textureData.height * 4;
	copyData.pixelData = calloc(1, numBytes);
	assert(copyData.pixelData != NULL);
	memcpy(copyData.pixelData, textureData.pixelData, numBytes);
	
	return copyData;
}
