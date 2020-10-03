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

#include "text.h"
#include "font.h"
#include "platforms.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

typedef struct
{
	TextureObject texture;
	char *text;
	int width;
	int height;
} TextRendering;

#define MAX_TEXT_RENDERING_COUNT 256

static int gTextRenderingCount = 0;
static TextRendering *gTextRenderings;

static BufferArrayObject gFontVertexAndTextureBufferObject;
static BufferObject gFontIndicesBufferObject;

void initText(Renderer *renderer)
{
	initFont();
	
	gTextRenderings = calloc(MAX_TEXT_RENDERING_COUNT, sizeof(*gTextRenderings));
	
	const ZGFloat verticesAndTextureCoordinates[] =
	{
		// vertices
		-1.0f, -1.0f, 0.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 0.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f,
		
		// texture coordinates
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f
	};
	
	gFontVertexAndTextureBufferObject = createVertexAndTextureCoordinateArrayObject(renderer, verticesAndTextureCoordinates, 16 * sizeof(*verticesAndTextureCoordinates), 8 * sizeof(*verticesAndTextureCoordinates));
	
	gFontIndicesBufferObject = rectangleIndexBufferObject(renderer);
}

void drawStringf(Renderer *renderer, mat4_t modelViewMatrix, color4_t color, ZGFloat width, ZGFloat height, const char *format, ...)
{
	va_list ap;
	char buffer[256];
	int formatIndex;
	int bufferIndex = 0;
	int stringIndex;
	char *string;
	
	va_start(ap, format);
	
	for (formatIndex = 0; format[formatIndex] != '\0'; formatIndex++)
	{
		if (format[formatIndex] != '%')
		{
			buffer[bufferIndex] = format[formatIndex];
			bufferIndex++;
			continue;
		}
		
		formatIndex++;
		
		int buffer_length;
		int spaces;
		
		switch (format[formatIndex])
		{
			case 'c':
				buffer[bufferIndex] = va_arg(ap, int);
				break;
			case 'd':
				bufferIndex += sprintf(&buffer[bufferIndex], "%i", va_arg(ap, int)) - 1;
				break;
			case 'i':
				bufferIndex += sprintf(&buffer[bufferIndex], "%i", va_arg(ap, int)) - 1;
				break;
			case '%':
				buffer[bufferIndex] = '%';
				break;
			case 'f':
				bufferIndex += sprintf(&buffer[bufferIndex], "%f", va_arg(ap, double)) - 1;
				break;
			case 's':
				string = va_arg(ap, char *);
				
				for (stringIndex = 0; string[stringIndex] != '\0'; stringIndex++)
				{
					buffer[bufferIndex++] = string[stringIndex];
				}
					
				bufferIndex--;
				break;
			case 'z':
				buffer_length = bufferIndex + 1;
				spaces = 4 - (buffer_length % 4);
				
				while (spaces != 0)
				{
					buffer[bufferIndex] = ' ';
					spaces--;
					bufferIndex++;
				}
				
				bufferIndex--;
				break;
		}
		
		bufferIndex++;
	}
	
	va_end(ap);
	
	buffer[bufferIndex] = '\0';
	
	drawString(renderer, modelViewMatrix, color, width, height, buffer);
}

#define MAX_TEXT_LENGTH 256
int cacheString(Renderer *renderer, const char *string)
{
	int cachedIndex = -1;
	int renderingCount = gTextRenderingCount < MAX_TEXT_RENDERING_COUNT ? gTextRenderingCount : MAX_TEXT_RENDERING_COUNT;
	for (int i = 0; i < renderingCount; i++)
	{
		if (strncmp(string, gTextRenderings[i].text, 256) == 0)
		{
			cachedIndex = i;
			break;
		}
	}
	
	if (cachedIndex == -1)
	{
		char *newText = calloc(MAX_TEXT_LENGTH, 1);
		if (newText != NULL)
		{
			// If we run past MAX_TEXT_RENDERING_COUNT, re-cycle through our old text renderings
			// and replace those
			int insertionIndex = gTextRenderingCount % MAX_TEXT_RENDERING_COUNT;
			
			if (gTextRenderings[insertionIndex].text != NULL)
			{
				free(gTextRenderings[insertionIndex].text);
				deleteTexture(renderer, gTextRenderings[insertionIndex].texture);
			}
			
			gTextRenderings[insertionIndex].text = newText;
			strncpy(gTextRenderings[insertionIndex].text, string, MAX_TEXT_LENGTH - 1);
			
			TextureData textData = createTextData(newText);
			gTextRenderings[insertionIndex].width = textData.width;
			gTextRenderings[insertionIndex].height = textData.height;
			gTextRenderings[insertionIndex].texture = textureFromPixelData(renderer, textData.pixelData, gTextRenderings[insertionIndex].width, gTextRenderings[insertionIndex].height, textData.pixelFormat);
			
			freeTextureData(textData);
			
			cachedIndex = insertionIndex;
			gTextRenderingCount++;
		}
	}
	
	return cachedIndex;
}

void drawString(Renderer *renderer, mat4_t modelViewMatrix, color4_t color, ZGFloat width, ZGFloat height, const char *string)
{
	int index = cacheString(renderer, string);
	if (index == -1) return;
	
	mat4_t scaleMatrix = m4_scaling((vec3_t){width, height, 0.0f});
	mat4_t transformMatrix = m4_mul(modelViewMatrix, scaleMatrix);
	
	drawTextureWithVerticesFromIndices(renderer, transformMatrix, gTextRenderings[index].texture, RENDERER_TRIANGLE_MODE, gFontVertexAndTextureBufferObject, gFontIndicesBufferObject, 6, color, RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA);
}

void drawStringScaled(Renderer *renderer, mat4_t modelViewMatrix, color4_t color, ZGFloat scale, const char *string)
{
	int index = cacheString(renderer, string);
	if (index == -1) return;
	
	int width = gTextRenderings[index].width;
	int height = gTextRenderings[index].height;
	
	mat4_t scaleMatrix = m4_scaling((vec3_t){width * scale, height * scale, 0.0f});
	mat4_t transformMatrix = m4_mul(modelViewMatrix, scaleMatrix);
	
	drawTextureWithVerticesFromIndices(renderer, transformMatrix, gTextRenderings[index].texture, RENDERER_TRIANGLE_MODE, gFontVertexAndTextureBufferObject, gFontIndicesBufferObject, 6, color, RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA);
}

void drawStringLeftAligned(Renderer *renderer, mat4_t modelViewMatrix, color4_t color, ZGFloat scale, const char *string)
{
	int index = cacheString(renderer, string);
	if (index == -1) return;
	
	int width = gTextRenderings[index].width;
	int height = gTextRenderings[index].height;
	
	mat4_t scaleMatrix = m4_scaling((vec3_t){width * scale, height * scale, 0.0f});
	mat4_t translationMatrix = m4_translation((vec3_t){width * scale, 0.0f, 0.0f});
	
	mat4_t transformMatrix = m4_mul(modelViewMatrix, m4_mul(translationMatrix, scaleMatrix));
	
	drawTextureWithVerticesFromIndices(renderer, transformMatrix, gTextRenderings[index].texture, RENDERER_TRIANGLE_MODE, gFontVertexAndTextureBufferObject, gFontIndicesBufferObject, 6, color, RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA);
}
