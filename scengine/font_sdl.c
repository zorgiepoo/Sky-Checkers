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

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "font.h"
#include "platforms.h"

static TTF_Font *gFont;

void initFontFromFile(const char *filePath, int pointSize)
{
	if (TTF_Init() == -1)
	{
		fprintf(stderr, "TTF_Init: %s or %s\n", TTF_GetError(), SDL_GetError());
		SDL_Quit();
	}
	
	gFont = TTF_OpenFont(filePath, pointSize);
	
	if (gFont == NULL)
	{
		fprintf(stderr, "loading font error! %s\n", TTF_GetError());
		SDL_Quit();
	}
}

TextureData createTextData(const char *string)
{
	SDL_Color color = {255, 255, 255, 0};

	SDL_Surface *fontSurface = TTF_RenderUTF8_Blended(gFont, string, color);

	if (fontSurface == NULL)
	{
		fprintf(stderr, "font surface is null: %s", TTF_GetError());
		SDL_Quit();
	}
	
	SDL_Surface *blittedSurface = SDL_CreateRGBSurface(0, fontSurface->w, fontSurface->h, 32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
#else
		0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
#endif
	);
	
	if (blittedSurface == NULL)
	{
		fprintf(stderr, "failed to create blitted surface: %s\n", SDL_GetError());
		SDL_Quit();
	}
	
	if (SDL_BlitSurface(fontSurface, NULL, blittedSurface, NULL) != 0)
	{
		fprintf(stderr, "font surface failed to blit: %s\n", SDL_GetError());
		SDL_Quit();
	}
	
	SDL_FreeSurface(fontSurface);
	
	return (TextureData){.pixelData = blittedSurface->pixels, .context = blittedSurface, .width = blittedSurface->w, .height = blittedSurface->h, .pixelFormat = PIXEL_FORMAT_RGBA32};
}
