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

static TextureObject gSkyTex;

typedef struct
{
	union
	{
		struct
		{
			TextureObject tile1;
			TextureObject tile2;
		};
		TextureArrayObject tiles;
	};
} TileTextures;

static TileTextures gTileTextures;

void initTiles(void)
{
	loadTiles();
	
	linkRow(0);
	linkRow(8);
	linkRow(16);
	linkRow(24);
	linkRow(32);
	linkRow(40);
	linkRow(48);
	linkRow(56);
	
	linkColumn(0);
	linkColumn(1);
	linkColumn(2);
	linkColumn(3);
	linkColumn(4);
	linkColumn(5);
	linkColumn(6);
	linkColumn(7);
}

void loadTiles(void)
{
	int tileIndex;
	
	for (tileIndex = 0; tileIndex < NUMBER_OF_TILES; tileIndex++)
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
SDL_bool availableTile(float x, float y)
{
	int tile_loc = getTileIndexLocation((int)x, (int)y);
	
	if (gTiles[tile_loc].state == SDL_FALSE)
		return SDL_FALSE;
	
	if (gTiles[tile_loc].z < TILE_ALIVE_Z)
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
	for (int i = 0; i < NUMBER_OF_TILES; i++)
	{
		if ((((i / 8) % 2) ^ (i % 2)) != 0)
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
	int startingTileIndex = 0;
	float y_loc = 12.5;
	
	do
	{
		float x_loc = -7.0;
		Tile *currentTile = &gTiles[startingTileIndex];
		
		do
		{
			currentTile->x = x_loc;
			currentTile->y = y_loc;
			currentTile->z = TILE_ALIVE_Z;
			
			x_loc += 2.0;
		}
		while ((currentTile = currentTile->right) != NULL); 
		
		y_loc += 2.0;
		startingTileIndex += 8;
	}
	while (startingTileIndex < 57);
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

void loadSceneryTextures(Renderer *renderer)
{
	gSkyTex = loadTexture(renderer, "Data/Textures/sky.bmp");
	
	if (renderer->supportsInstancing)
	{
		gTileTextures.tiles = load2DTextureArray(renderer, "Data/Textures/tiletex.bmp", "Data/Textures/tiletex2.bmp");
	}
	else
	{
		gTileTextures.tile1 = loadTexture(renderer, "Data/Textures/tiletex.bmp");
		gTileTextures.tile2 = loadTexture(renderer, "Data/Textures/tiletex2.bmp");
	}
}

void drawSky(Renderer *renderer)
{
	static BufferArrayObject vertexAndTextureArrayObject;
	static BufferObject indicesBufferObject;
	static SDL_bool initializedBuffers;
	
	if (!initializedBuffers)
	{
		uint16_t indices[] =
		{
			0, 1, 2,
			2, 3, 0
		};
		
		float vertexAndTextureCoordinates[] =
		{
			// vertices
			-16.0f, 16.0f, 0.0f,
			16.0f, 16.0f, 0.0f,
			16.0f, -16.0f, 0.0f,
			-16.0f, -16.0f, 0.0f,
			
			// texture coordinates
			0.0f, 0.0f,
			1.0f, 0.0f,
			1.0f, 1.0f,
			0.0f, 1.0f
		};
		
		vertexAndTextureArrayObject = createVertexAndTextureCoordinateArrayObject(renderer, vertexAndTextureCoordinates, 12 * sizeof(*vertexAndTextureCoordinates), 8 * sizeof(*vertexAndTextureCoordinates));
		indicesBufferObject = createBufferObject(renderer, indices, sizeof(indices));
		
		initializedBuffers = SDL_TRUE;
	}
	
	mat4_t modelViewMatrix = m4_translation((vec3_t){0.0f, 0.0f, -38.0f});
	
	drawTextureWithVerticesFromIndices(renderer, modelViewMatrix, gSkyTex, RENDERER_TRIANGLE_MODE, vertexAndTextureArrayObject, indicesBufferObject, 6, (color4_t){1.0f, 1.0f, 1.0f, 0.9f}, RENDERER_OPTION_BLENDING_ALPHA);
}

void drawTiles(Renderer *renderer)
{
	static BufferArrayObject vertexAndTextureCoordinateArrayObject;
	static BufferObject indicesBufferObject;
	static uint32_t textureIndices[NUMBER_OF_TILES];
	static SDL_bool supportsInstancing;
	static SDL_bool initializedBuffers;
	
	if (!initializedBuffers)
	{
		float verticesAndTextureCoordinates[] =
		{
			// Vertices
			
			// Bottom
			-1.0f, -1.0f, 1.0f,
			-1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, 1.0f,
			
			// Left
			-1.0f, 1.0f, 1.0f,
			-1.0f, 1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f, 1.0f,
			
			// Right
			1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, -1.0,
			1.0f, -1.0f, -1.0,
			1.0f, -1.0f, 1.0f,
			
			// Front
			-1.0f, 1.0f, 1.0f,
			-1.0f, -1.0f, 1.0f,
			1.0f, -1.0f, 1.0f,
			1.0f, 1.0f, 1.0f,
			
			// Texture coordinates
			// Bottom
			0.0f, 1.0f,
			0.0f, 0.0f,
			1.0f, 0.0f,
			1.0f, 1.0f,
			
			// Left
			1.0f, 0.0f,
			0.0f, 0.0f,
			0.0f, 1.0f,
			1.0f, 1.0f,
			
			// Right
			0.0f, 0.0f,
			1.0f, 0.0f,
			1.0f, 1.0f,
			0.0f, 1.0f,
			
			// Front
			0.0f, 0.0f,
			0.0f, 1.0f,
			1.0f, 1.0f,
			1.0f, 0.0f,
		};
		
		const uint16_t indices[] =
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
		
		vertexAndTextureCoordinateArrayObject = createVertexAndTextureCoordinateArrayObject(renderer, verticesAndTextureCoordinates, sizeof(*verticesAndTextureCoordinates) * 48, sizeof(*verticesAndTextureCoordinates) * 32);
		
		indicesBufferObject = createBufferObject(renderer, indices, sizeof(indices));
		
		supportsInstancing = renderer->supportsInstancing;
		
		if (supportsInstancing)
		{
			for (int i = 0; i < NUMBER_OF_TILES; i++)
			{
				uint32_t textureIndex = (((i / 8) % 2) ^ (i % 2)) != 0 ? 0 : 1;
				textureIndices[i] = textureIndex;
			}
		}
		
		initializedBuffers = SDL_TRUE;
	}
	
	mat4_t worldRotationMatrix = m4_rotation_x(-40.0f * ((float)M_PI / 180.0f));
	if (supportsInstancing)
	{
		mat4_t projectionMatrix = renderer->projectionMatrix;
		
		mat4_t modelViewProjectionMatrices[NUMBER_OF_TILES];
		color4_t colors[NUMBER_OF_TILES];
		
		for (int i = 0; i < NUMBER_OF_TILES; i++)
		{
			mat4_t modelTranslationMatrix = m4_translation((vec3_t){gTiles[i].x , gTiles[i].y, gTiles[i].z});
			mat4_t modelViewProjectionMatrix = m4_mul(projectionMatrix, m4_mul(worldRotationMatrix, modelTranslationMatrix));
			
			modelViewProjectionMatrices[i] = modelViewProjectionMatrix;
			colors[i] = (color4_t){gTiles[i].red, gTiles[i].green, gTiles[i].blue, 1.0f};
		}
		
		drawInstancedTextureArrayWithVerticesFromIndices(renderer, modelViewProjectionMatrices, gTileTextures.tiles, colors, textureIndices, RENDERER_TRIANGLE_MODE, vertexAndTextureCoordinateArrayObject, indicesBufferObject, 24, NUMBER_OF_TILES, RENDERER_OPTION_NONE);
	}
	else
	{
		for (int i = 0; i < NUMBER_OF_TILES; i++)
		{
			mat4_t modelTranslationMatrix = m4_translation((vec3_t){gTiles[i].x , gTiles[i].y, gTiles[i].z});
			mat4_t modelViewMatrix = m4_mul(worldRotationMatrix, modelTranslationMatrix);
			
			TextureObject texture = (((i / 8) % 2) ^ (i % 2)) != 0 ? gTileTextures.tile1 : gTileTextures.tile2;
			
			drawTextureWithVerticesFromIndices(renderer, modelViewMatrix, texture, RENDERER_TRIANGLE_MODE, vertexAndTextureCoordinateArrayObject, indicesBufferObject, 24, (color4_t){gTiles[i].red, gTiles[i].green, gTiles[i].blue, 1.0f}, RENDERER_OPTION_NONE);
		}
	}
}
