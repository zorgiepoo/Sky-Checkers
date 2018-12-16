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

#include "font.h"
#include "utilities.h"

TextureObject loadString(Renderer *renderer, const char *string);

/* Glyph structure */
typedef struct
{
	TextureObject texture;
	char *text;
	int width;
	int height;
} Glyph;

static TTF_Font *gFont;

// records how many glyphs we've loaded with a counter
static int gGlyphsCounter =	0;
// gMaxGlyphs may be incremented by 256 if it needs to resize itself
static int gMaxGlyphs =		256;
static Glyph *gGlyphs;

static BufferArrayObject gFontVertexAndTextureBufferObject;
static BufferObject gFontIndicesBufferObject;

static int lookUpGlyphIndex(const char *string);

SDL_bool initFont(Renderer *renderer)
{
	if (TTF_Init() == -1)
	{
		zgPrint("TTF_Init: %t or %e");
		return SDL_FALSE;
	}
	
	// This font is "goodfish.ttf" and is intentionally obfuscated in source by author's request
	// A license to embed the font was acquired (for me, Mayur, only) from http://typodermicfonts.com/goodfish/
	gFont = TTF_OpenFont("Data/Fonts/typelib.dat", 144);
	
	if (gFont == NULL)
	{
		zgPrint("loading font error! %t");
		SDL_Quit();
	}
	
	gGlyphs = malloc(sizeof(Glyph) * gMaxGlyphs);
	
	for (int i = 0; i < gMaxGlyphs; i++)
	{
		gGlyphs[i].text = NULL;
	}
	
	const uint16_t indices[] =
	{
		0, 1, 2,
		2, 3, 0
	};
	
	const float verticesAndTextureCoordinates[] =
	{
		// vertices
		-1.0f, -1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		
		// texture coordinates
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f
	};
	
	gFontVertexAndTextureBufferObject = createVertexAndTextureCoordinateArrayObject(renderer, verticesAndTextureCoordinates, 12 * sizeof(*verticesAndTextureCoordinates), 8 * sizeof(*verticesAndTextureCoordinates));
	
	gFontIndicesBufferObject = createBufferObject(renderer, indices, sizeof(indices));
	
	return SDL_TRUE;
}

// returns a texture to draw for the string.
TextureObject loadString(Renderer *renderer, const char *string)
{
	SDL_Color color = {255, 255, 255, 0};
	
	// TTF_RenderText_Solid always returns NULL for me, so use TTF_RenderText_Blended.
	SDL_Surface *fontSurface = TTF_RenderText_Blended(gFont, string, color);
	
	if (fontSurface == NULL)
	{
		zgPrint("font surface is null: %t");
		SDL_Quit();
	}
	
	TextureObject texture = surfaceToTexture(renderer, fontSurface);
	
	// Cleanup
	SDL_FreeSurface(fontSurface);
	
	return texture;
}

// Sees if character can be found in the collection of compiled glyphs.
// If so, it returns the index that it is in the glyphs array.
// otherwise, returns -1 if the glyph index can't be found, which will probably mean we'll
// want to compile the character later on and add it to the collection of compiled glyphs.
static int lookUpGlyphIndex(const char *string)
{	
	int i;
	int index = -1;
	
	if (gGlyphsCounter == 0)
		return index;
	
	for (i = 0; i <= gGlyphsCounter; i++)
	{
		if (gGlyphs[i].text != NULL && strcmp(gGlyphs[i].text, string) == 0)
		{
			index = i;
			break;
		}
	}
	
	return index;
}

// partially copied from zgPrint(...)
void drawStringf(Renderer *renderer, mat4_t modelViewMatrix, color4_t color, float width, float height, const char *format, ...)
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

int cacheString(Renderer *renderer, const char *string)
{
	int index = lookUpGlyphIndex(string);
	if (index == -1)
	{
		// Add the new glyph and cache it so we can re-use it.
		if (gGlyphsCounter == gMaxGlyphs)
		{
			gMaxGlyphs += 256;
			gGlyphs = realloc(gGlyphs, gMaxGlyphs);
		}
		
		gGlyphs[gGlyphsCounter].text = calloc(256, 1);
		strncpy(gGlyphs[gGlyphsCounter].text, string, 256 - 1);
		gGlyphs[gGlyphsCounter].texture = loadString(renderer, gGlyphs[gGlyphsCounter].text);
		TTF_SizeUTF8(gFont, gGlyphs[gGlyphsCounter].text, &gGlyphs[gGlyphsCounter].width, &gGlyphs[gGlyphsCounter].height);
		
		gGlyphsCounter++;
		
		return (gGlyphsCounter - 1);
	}
	
	return index;
}

void drawString(Renderer *renderer, mat4_t modelViewMatrix, color4_t color, float width, float height, const char *string)
{
	int index = cacheString(renderer, string);
	
	mat4_t scaleMatrix = m4_scaling((vec3_t){width, height, 0.0f});
	mat4_t transformMatrix = m4_mul(modelViewMatrix, scaleMatrix);
	
	drawTextureWithVerticesFromIndices(renderer, transformMatrix, gGlyphs[index].texture, RENDERER_TRIANGLE_MODE, gFontVertexAndTextureBufferObject, gFontIndicesBufferObject, 6, color, RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA | RENDERER_OPTION_DISABLE_DEPTH_TEST);
}

void drawStringScaled(Renderer *renderer, mat4_t modelViewMatrix, color4_t color, float scale, const char *string)
{
	int index = cacheString(renderer, string);
	
	int width = gGlyphs[index].width;
	int height = gGlyphs[index].height;
	
	mat4_t scaleMatrix = m4_scaling((vec3_t){width * scale, height * scale, 0.0f});
	mat4_t transformMatrix = m4_mul(modelViewMatrix, scaleMatrix);
	
	drawTextureWithVerticesFromIndices(renderer, transformMatrix, gGlyphs[index].texture, RENDERER_TRIANGLE_MODE, gFontVertexAndTextureBufferObject, gFontIndicesBufferObject, 6, color, RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA | RENDERER_OPTION_DISABLE_DEPTH_TEST);
}

void drawStringLeftAligned(Renderer *renderer, mat4_t modelViewMatrix, color4_t color, float scale, const char *string)
{
	int index = cacheString(renderer, string);
	
	int width = gGlyphs[index].width;
	int height = gGlyphs[index].height;
	
	mat4_t scaleMatrix = m4_scaling((vec3_t){width * scale, height * scale, 0.0f});
	mat4_t translationMatrix = m4_translation((vec3_t){width * scale, 0.0f, 0.0f});
	
	mat4_t transformMatrix = m4_mul(modelViewMatrix, m4_mul(translationMatrix, scaleMatrix));
	
	drawTextureWithVerticesFromIndices(renderer, transformMatrix, gGlyphs[index].texture, RENDERER_TRIANGLE_MODE, gFontVertexAndTextureBufferObject, gFontIndicesBufferObject, 6, color, RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA | RENDERER_OPTION_DISABLE_DEPTH_TEST);
}
