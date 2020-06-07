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

#include "font.h"
#include "platforms.h"

#include <SDL2/SDL_ttf.h>

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

	// TTF_RenderText_Solid always returns NULL for me, so use TTF_RenderText_Blended.
	SDL_Surface *fontSurface = TTF_RenderText_Blended(gFont, string, color);

	if (fontSurface == NULL)
	{
		fprintf(stderr, "font surface is null: %s", TTF_GetError());
		SDL_Quit();
	}
	
	return (TextureData){.pixelData = fontSurface->pixels, .context = fontSurface, .width = fontSurface->w, .height = fontSurface->h, .pixelFormat = PIXEL_FORMAT_BGRA32};
}
