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

GLuint loadString(char *string);

/* Glyph structure */
typedef struct
{
	GLuint texture;
	char *text;
} Glyph;

static GLfloat gFontVertices[] =
{
	-1.0f, -1.0f,
	-1.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, -1.0f
};

static GLfloat gFontTextureCoordinates[] =
{
	0.0f, 1.0f,
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f
};

static GLubyte gFontIndices[] =
{
	0, 1, 2,
	2, 3, 0
};

// records how many glyphs we've loaded with a counter
static int gGlyphsCounter =	0;
// gMaxGlyphs may be incremented by 256 if it needs to resize itself
static int gMaxGlyphs =		256;
static Glyph *gGlyphs;

static int lookUpGlyphIndex(char *string, GLfloat width, GLfloat height);

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
GLuint loadString(char *string)
{	
	if (string == NULL)
		return 0;
	
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
	
	GLuint texture = 0;
	
	// generate texture
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	// glTexImage2D should accept non-power-of-two sizes in GL 2.0 or later
	glTexImage2D(GL_TEXTURE_2D,
				 0,
				 GL_RGBA,
				 textSurface->w, textSurface->h,
				 0,
				 GL_RGBA,
				 GL_UNSIGNED_BYTE,
				 textSurface->pixels);
	
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
static int lookUpGlyphIndex(char *string, GLfloat width, GLfloat height)
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
void drawStringf(GLfloat width, GLfloat height, const char *format, ...)
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
	
	drawString(width, height, buffer);
}

void drawString(GLfloat width, GLfloat height, char *string)
{	
	if (string == NULL)
		return;
	
	int index = lookUpGlyphIndex(string, width, height);
	
	if (index == -1)
	{
		// Add the new glyph and cache it so we can re-use it.
		// Then proceed on into drawing it.
		
		if (gGlyphsCounter == gMaxGlyphs)
		{
			gMaxGlyphs += 256;
			gGlyphs = realloc(gGlyphs, gMaxGlyphs);
		}
		
		gGlyphs[gGlyphsCounter].text = malloc(strlen(string) + 1);
		sprintf(gGlyphs[gGlyphsCounter].text, "%s", string);
		gGlyphs[gGlyphsCounter].texture = loadString(gGlyphs[gGlyphsCounter].text);
		
		gGlyphsCounter++;
		
		// Now that we've loaded the new glyph, wait till the next time this function is called.
		// If we don't wait, there may be a graphics glitch in drawing the glyph/string.
		return;
	}
	
	// Need to disable DEPTH_TEST, otherwise the glyph's context will die if the OpenGL scene is resized.
	glDisable(GL_DEPTH_TEST);
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, gGlyphs[index].texture);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glVertexPointer(2, GL_FLOAT, 0, gFontVertices);
	glTexCoordPointer(2, GL_FLOAT, 0, gFontTextureCoordinates);
	
	glScalef(width, height, 0.0f);
	glDrawElements(GL_TRIANGLES, sizeof(gFontIndices) / sizeof(*gFontIndices), GL_UNSIGNED_BYTE, gFontIndices);
	glScalef(1.0f / width, 1.0f / height, 0.0f);
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glDisable(GL_BLEND);
	
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
}
