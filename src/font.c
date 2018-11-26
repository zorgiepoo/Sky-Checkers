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
} Glyph;

// records how many glyphs we've loaded with a counter
static int gGlyphsCounter =	0;
// gMaxGlyphs may be incremented by 256 if it needs to resize itself
static int gMaxGlyphs =		256;
static Glyph *gGlyphs;

static int lookUpGlyphIndex(const char *string);

SDL_bool initFont(void)
{
	int i;
	
	if (TTF_Init() == -1)
	{
		zgPrint("TTF_Init: %t or %e");
		return SDL_FALSE;
	}
	
	gGlyphs = malloc(sizeof(Glyph) * gMaxGlyphs);
	
	for (i = 0; i < gMaxGlyphs; i++)
	{
		gGlyphs[i].text = NULL;
	}
	
	return SDL_TRUE;
}

// returns a texture to draw for the string.
TextureObject loadString(Renderer *renderer, const char *string)
{
	// This font is "goodfish.ttf" and is intentionally obfuscated in source by author's request
	// A license to embed the font was acquired (for me, Mayur, only) from http://typodermicfonts.com/goodfish/
	static const char *FONT_PATH = "Data/Fonts/typelib.dat";
	
	// load font..
	TTF_Font *font = TTF_OpenFont(FONT_PATH, 144);
	
	if (!font)
		zgPrint("loading font error! %t");
	
	SDL_Color color = {255, 255, 255, 0};
	
	// TTF_RenderText_Solid always returns NULL for me, so use TTF_RenderText_Blended.
	SDL_Surface *fontSurface = TTF_RenderText_Blended(font, string, color);
	
	if (fontSurface == NULL)
		zgPrint("font surface is null: %t");
	
	Uint32 rmask, gmask, bmask, amask;
	
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
#endif
	
	SDL_Surface *textSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, fontSurface->w, fontSurface->h, 32, rmask, gmask, bmask, amask);
	
	if (textSurface == NULL)
		zgPrint("SDL_CreateRGBSurface failed: %e");
	
	SDL_SetSurfaceAlphaMod(fontSurface, 255);
	
	if (SDL_BlitSurface(fontSurface, NULL, textSurface, NULL) == -1)
		zgPrint("blitting failed");
	
	TextureObject texture = textureFromPixelData(renderer, textSurface->pixels, textSurface->w, textSurface->h);
	
	// Cleanup
	SDL_FreeSurface(fontSurface);
	SDL_FreeSurface(textSurface);
	TTF_CloseFont(font);
	
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
	if (string == NULL)
	{
		return -1;
	}
	
	int index = lookUpGlyphIndex(string);
	if (index == -1)
	{
		// Add the new glyph and cache it so we can re-use it.
		if (gGlyphsCounter == gMaxGlyphs)
		{
			gMaxGlyphs += 256;
			gGlyphs = realloc(gGlyphs, gMaxGlyphs);
		}
		
		gGlyphs[gGlyphsCounter].text = malloc(strlen(string) + 1);
		sprintf(gGlyphs[gGlyphsCounter].text, "%s", string);
		gGlyphs[gGlyphsCounter].texture = loadString(renderer, gGlyphs[gGlyphsCounter].text);
		
		gGlyphsCounter++;
	}
	
	// If we just loaded the glyph, we will return an invalid index and wait the next time this is called
	// to avoid potential glitches
	return index;
}

void drawString(Renderer *renderer, mat4_t modelViewMatrix, color4_t color, float width, float height, const char *string)
{
	if (string == NULL)
	{
		return;
	}
	
	int index = cacheString(renderer, string);
	if (index == -1)
	{
		return;
	}
	
	static BufferArrayObject vertexAndTextureBufferObject;
	static BufferObject indicesBufferObject;
	static SDL_bool initializedBuffers;
	
	if (!initializedBuffers)
	{
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
		
		vertexAndTextureBufferObject = createVertexAndTextureCoordinateArrayObject(renderer, verticesAndTextureCoordinates, 12 * sizeof(*verticesAndTextureCoordinates), 8 * sizeof(*verticesAndTextureCoordinates));
		
		indicesBufferObject = createBufferObject(renderer, indices, sizeof(indices));
		
		initializedBuffers = SDL_TRUE;
	}
	
	mat4_t scaleMatrix = m4_scaling((vec3_t){width, height, 0.0f});
	mat4_t transformMatrix = m4_mul(modelViewMatrix, scaleMatrix);
	
	drawTextureWithVerticesFromIndices(renderer, transformMatrix, gGlyphs[index].texture, RENDERER_TRIANGLE_MODE, vertexAndTextureBufferObject, indicesBufferObject, 6, color, RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA | RENDERER_OPTION_DISABLE_DEPTH_TEST);
}
