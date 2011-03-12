/*
 * Coder: Mayur Pawashe (Zorg)
 * http://zorg.tejat.net
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
