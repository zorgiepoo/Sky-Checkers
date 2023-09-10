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

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "font.h"
#include "platforms.h"

static TTF_Font *gFont;

void initFont(void)
{
	if (TTF_Init() == -1)
	{
		fprintf(stderr, "TTF_Init: %s or %s\n", TTF_GetError(), SDL_GetError());
		SDL_Quit();
	}
	
	gFont = TTF_OpenFont(FONT_PATH, FONT_POINT_SIZE);
	
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
