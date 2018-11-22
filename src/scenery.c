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

#include "scenery.h"
#include "utilities.h"
#include "console.h"
#include "collision.h"
#include "math_3d.h"

static void loadTileLocations(void);
static void loadTileColors(void);

static void linkRow(int index);
static void linkColumn(int index);

// starts at index '1' for all of our calculations, 1 to 64
Tile gTiles[65];

static GLuint gSkyTex;
static GLuint gTileOneTex;
static GLuint gTileTwoTex;

void initTiles(void)
{
	loadTiles();
	
	linkRow(1);
	linkRow(9);
	linkRow(17);
	linkRow(25);
	linkRow(33);
	linkRow(41);
	linkRow(49);
	linkRow(57);
	
	linkColumn(1);
	linkColumn(2);
	linkColumn(3);
	linkColumn(4);
	linkColumn(5);
	linkColumn(6);
	linkColumn(7);
	linkColumn(8);
}

void loadTiles(void)
{
	int tileIndex;
	
	for (tileIndex = 1; tileIndex <= 64; tileIndex++)
	{
		gTiles[tileIndex].state = SDL_TRUE;
		gTiles[tileIndex].recovery_timer = 0;
		gTiles[tileIndex].isDead = SDL_FALSE;
	}
	
	loadTileLocations();
	loadTileColors();
}

/*
 * Tests to see if the given x and y coordinates are available for a tile.
 * Returns SDL_FALSE if the tile is not free to use, otherwise returns SDL_TRUE
 */
SDL_bool availableTile(GLfloat x, GLfloat y)
{
	int tile_loc = getTileIndexLocation((int)x, (int)y);
	
	if (gTiles[tile_loc].state == SDL_FALSE)
		return SDL_FALSE;
	
	if (gTiles[tile_loc].z != -25.0)
		return SDL_FALSE;
	
	if (gTiles[tile_loc].red != gTiles[tile_loc].d_red || gTiles[tile_loc].blue != gTiles[tile_loc].d_blue || gTiles[tile_loc].d_green != gTiles[tile_loc].d_green)
		return SDL_FALSE;
	
	if (getTileIndexLocation((int)gRedRover.x, (int)gRedRover.y) == tile_loc)
		return SDL_FALSE;
	
	if (getTileIndexLocation((int)gGreenTree.x, (int)gGreenTree.y) == tile_loc)
		return SDL_FALSE;
	
	if (getTileIndexLocation((int)gPinkBubbleGum.x, (int)gPinkBubbleGum.y) == tile_loc)
		return SDL_FALSE;
	
	if (getTileIndexLocation((int)gBlueLightning.x, (int)gBlueLightning.y) == tile_loc)
		return SDL_FALSE;
	
	return SDL_TRUE;
}

static void loadTileColors(void)
{
	int i;
	SDL_bool drawFirst;
	
	for (i = 1; i <= 64; i++)
	{
		
		if ((i >= 1 && i <= 8) || (i >= 17 && i <= 24) || (i >= 33 && i <= 40) || (i >= 49 && i <= 56))
		{
			drawFirst = SDL_TRUE;
		}
		else
		{
			drawFirst = SDL_FALSE;
		}
		
		if ((drawFirst && i % 2 == 0) || (!drawFirst && i % 2 == 1))
		{
			gTiles[i].red = 0.8f;
			gTiles[i].green = 0.8f;
			gTiles[i].blue = 0.8f;
			gTiles[i].d_red = 0.8f;
			gTiles[i].d_green = 0.8f;
			gTiles[i].d_blue = 0.8f;
		}
		else
		{
			gTiles[i].red = 0.682f;
			gTiles[i].green = 0.572f;
			gTiles[i].blue = 0.329f;
			gTiles[i].d_red = 0.682f;
			gTiles[i].d_green = 0.572f;
			gTiles[i].d_blue = 0.329f;
		}
	}
}

static void loadTileLocations(void)
{	
	int startingTileIndex = 1;
	GLfloat y_loc = 12.5;
	
	do
	{
		GLfloat x_loc = -7.0;
		Tile *currentTile = &gTiles[startingTileIndex];
		
		do
		{
			currentTile->x = x_loc;
			currentTile->y = y_loc;
			currentTile->z = -25.0;
			
			x_loc += 2.0;
		}
		while ((currentTile = currentTile->right) != NULL); 
		
		y_loc += 2.0;
		startingTileIndex += 8;
	}
	while (startingTileIndex <= 57);
}

static void linkRow(int index)
{
	// link the rights.
	int last = index + 7;
	
	while (index < last)
	{
		gTiles[index].right = &gTiles[index + 1];
		
		index++;
	}
	
	gTiles[index].right = NULL;
	
	// link the lefts.
	last = index - 7;
	
	while (last < index)
	{
		gTiles[index].left = &gTiles[index - 1];
		
		index--;
	}
	
	gTiles[index].left = NULL;
}

static void linkColumn(int index)
{
	// link the ups.
	int last = index + (8 * 7);
	
	while (index < last)
	{
		gTiles[index].up = &gTiles[index + 8];
		
		index += 8;
	}
	
	gTiles[index].up = NULL;
	
	// link the downs.
	last = last - (8 * 7);
	
	while (last < index)
	{
		gTiles[index].down = &gTiles[index - 8];
		
		index -= 8;
	}
	
	gTiles[index].down = NULL;
}

void loadSceneryTextures(void)
{
	loadTexture("Data/Textures/sky.bmp", &gSkyTex);
	loadTexture("Data/Textures/tiletex.bmp", &gTileOneTex);
	loadTexture("Data/Textures/tiletex2.bmp", &gTileTwoTex);
}

void drawSky(void)
{	
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_SRC_ALPHA);
	
	mat4_t modelViewMatrix = m4_translation((vec3_t){0.0f, 0.0f, -25.0f});
	glLoadMatrixf(&modelViewMatrix.m00);
	
	glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
	
	GLfloat skyVertices[] =
	{
		-16.0f, 16.0f, -13.0f,
		16.0f, 16.0f, -13.0f,
		16.0f, -16.0f, -13.0f,
		-16.0f, -16.0f, -13.0f,
	};
	
	GLfloat skyTextureCoordinates[] =
	{
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
	};
	
	GLubyte indices[] =
	{
		0, 1, 2,
		2, 3, 0
	};
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glBindTexture(GL_TEXTURE_2D, gSkyTex);
	glVertexPointer(3, GL_FLOAT, 0, skyVertices);
	glTexCoordPointer(2, GL_FLOAT, 0, skyTextureCoordinates);
	
	glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(*indices), GL_UNSIGNED_BYTE, indices);
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}

void drawTiles(void)
{
	glEnable(GL_TEXTURE_2D);
	
	SDL_bool drawTileOneFirst = SDL_FALSE;
	
	mat4_t worldRotationMatrix = m4_rotation_x(-40.0f * (M_PI / 180.0f));
	
	for (int i = 1; i <= 64; i++)
	{
		mat4_t modelTranslationMatrix = m4_translation((vec3_t){gTiles[i].x , gTiles[i].y, gTiles[i].z});
		mat4_t modelViewMatrix = m4_mul(worldRotationMatrix, modelTranslationMatrix);
		glLoadMatrixf(&modelViewMatrix.m00);
		
		// If it's at an odd row number, set drawTileOneFirst to TRUE, otherwise set it to FALSE.
		if ((i >= 1 && i <= 8) || (i >= 17 && i <= 24) || (i >= 33 && i <= 40) || (i >= 49 && i <= 56))
		{
			drawTileOneFirst = SDL_TRUE;
		}
		else
		{
			drawTileOneFirst = SDL_FALSE;
		}
		
		glColor3f(gTiles[i].red, gTiles[i].green, gTiles[i].blue);
		
		// Figure out which texture to bind to
		if ((drawTileOneFirst && i % 2 == 0) || (!drawTileOneFirst && i % 2 == 1))
		{
			glBindTexture(GL_TEXTURE_2D, gTileOneTex);
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, gTileTwoTex);
		}
		
		// Draw the tile
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		
		GLfloat vertices[] =
		{
			// Bottom
			-1.0f, -1.0f, 1.0f,
			-1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, 1.0f,
			
			// Left
			-1.0f, 1.0f, 1.0f,
			-1.0, 1.0, -1.0,
			-1.0, -1.0, -1.0,
			-1.0, -1.0, 1.0,
			
			// Right
			1.0, 1.0, 1.0,
			1.0, 1.0, -1.0,
			1.0, -1.0, -1.0,
			1.0, -1.0, 1.0,
			
			// Front
			-1.0, 1.0, 1.0,
			-1.0, -1.0, 1.0,
			1.0, -1.0, 1.0,
			1.0, 1.0, 1.0,
		};
		
		glVertexPointer(3, GL_FLOAT, 0, vertices);
		
		GLshort textureCoordinates[] =
		{
			// Bottom
			0, 1,
			0, 0,
			1, 0,
			1, 1,
			
			// Left
			1, 0,
			0, 0,
			0, 1,
			1, 1,
			
			// Right
			0, 0,
			1, 0,
			1, 1,
			0, 1,
			
			// Front
			0, 0,
			0, 1,
			1, 1,
			1, 0,
		};
		
		glTexCoordPointer(2, GL_SHORT, 0, textureCoordinates);
		
		GLubyte indices[] =
		{
			// Bottom
			0, 1, 2,
			2, 3, 0,
			
			// Left
			4, 5, 6,
			6, 7, 4,
			
			// Right
			8, 9, 10,
			10, 11, 8,
			
			// Front
			12, 13, 14,
			14, 15, 12
		};
		
		glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(*indices), GL_UNSIGNED_BYTE, indices);
		
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	
	glDisable(GL_TEXTURE_2D);
}
