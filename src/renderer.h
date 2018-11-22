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

#include "maincore.h"
#include "math_3d.h"

typedef struct
{
	SDL_Window *window;
	mat4_t projectionMatrix;
	
	int32_t screenWidth;
	int32_t screenHeight;
	
	int32_t windowWidth;
	int32_t windowHeight;
	
	SDL_bool vsync;
	SDL_bool fsaa;
} Renderer;

typedef struct
{
	float red;
	float green;
	float blue;
	float alpha;
} color4_t;

void createRenderer(Renderer *renderer, int32_t windowWidth, int32_t windowHeight, uint32_t videoFlags, SDL_bool vsync, SDL_bool fsaa);

void swapBuffers(Renderer *renderer);

void loadTexture(Renderer *renderer, const char *filePath, uint32_t *tex);
uint32_t textureFromPixelData(Renderer *renderer, const void *pixels, int32_t width, int32_t height);
