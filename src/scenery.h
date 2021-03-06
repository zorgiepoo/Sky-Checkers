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

#include "renderer.h"

#define TILE_ALIVE_Z -25.0f
#define TILE_TERMINATING_Z -105.0f
#define NUMBER_OF_TILES 64

#define DIEING_STONE_ID 5

typedef struct
{
	float x;
	float y;
	float z;
	float red, green, blue;
	
	// ID of character that colored tile or DIEING_STONE_ID
	int coloredID;
	float colorTime;
	
	// Same as above but used by client prediction for netcode
	int predictedColorID;
	float predictedColorTime;
	
	float crackedTime;
	
	bool state;
	bool isDead;
	bool cracked;
	
	double recovery_timer;
} Tile;

extern Tile gTiles[NUMBER_OF_TILES];

void loadTiles(void);

void restoreDefaultTileColor(int tileIndex);
void setDieingTileColor(int tileIndex);

void clearPredictedColor(int tileIndex);
void clearPredictedColorWithTime(int tileIndex, float currentTime);
void clearPredictedColorsForCharacter(int characterID);

// Returns -1 if no such tile is found
int rightTileIndex(int tileIndex);
int leftTileIndex(int tileIndex);
int upTileIndex(int tileIndex);
int downTileIndex(int tileIndex);

bool availableTileIndex(int tileIndex);

void loadSceneryTextures(Renderer *renderer);

void drawSky(Renderer *renderer, RendererOptions options);
void drawTiles(Renderer *renderer);
