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

static int gTextRenderingCount = 0;
static int gTextRenderingCacheMaxCount;
static TextRendering *gTextRenderings;

static BufferArrayObject gFontVertexAndTextureBufferObject;
static BufferObject gFontIndicesBufferObject;

void initText(Renderer *renderer, int textRenderingCacheCount)
{
	gTextRenderings = calloc(textRenderingCacheCount, sizeof(*gTextRenderings));
    gTextRenderingCacheMaxCount = textRenderingCacheCount;
    
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

#if SUPPORT_DEPRECATED_DRAW_STRING_APIS
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
#endif

#define MAX_TEXT_LENGTH 256
int cacheString(Renderer *renderer, const char *string)
{
	int cachedIndex = -1;
	int renderingCount = gTextRenderingCount < gTextRenderingCacheMaxCount ? gTextRenderingCount : gTextRenderingCacheMaxCount;
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
			// If we run past gTextRenderingCacheMaxCount, re-cycle through our old text renderings
			// and replace those
			int insertionIndex = gTextRenderingCount % gTextRenderingCacheMaxCount;
			
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

#if SUPPORT_DEPRECATED_DRAW_STRING_APIS
void drawString(Renderer *renderer, mat4_t modelViewMatrix, color4_t color, ZGFloat width, ZGFloat height, const char *string)
{
	int index = cacheString(renderer, string);
	if (index == -1) return;
	
	mat4_t scaleMatrix = m4_scaling((vec3_t){width, height, 0.0f});
	mat4_t transformMatrix = m4_mul(modelViewMatrix, scaleMatrix);
	
	drawTextureWithVerticesFromIndices(renderer, transformMatrix, gTextRenderings[index].texture, RENDERER_TRIANGLE_MODE, gFontVertexAndTextureBufferObject, gFontIndicesBufferObject, 6, color, RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA);
}
#endif

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
