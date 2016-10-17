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

typedef struct _tile
{
	GLfloat x;
	GLfloat y;
	GLfloat z;
	struct _tile *right;
	struct _tile *left;
	struct _tile *down;
	struct _tile *up;
	GLfloat red, green, blue;
	// default colors
	GLfloat d_red, d_green, d_blue;
	
	SDL_bool state;
	SDL_bool isDead;
	
	int recovery_timer;
} Tile;

/* Tiles start from 1 instead of 0 */
extern Tile gTiles[65];

void initTiles(void);
void loadTiles(void);

SDL_bool availableTile(GLfloat x, GLfloat y);

void deleteSceneryTextures(void);
void loadSceneryTextures(void);

void drawSky(void);
void drawTiles(void);
