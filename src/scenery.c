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
#include "collision.h"
#include "math_3d.h"
#include "texture.h"
#include "renderer_projection.h"

#define TILE_TEXTURE1_RED 0.8f
#define TILE_TEXTURE1_GREEN 0.8f
#define TILE_TEXTURE1_BLUE 0.8f

#define TILE_TEXTURE2_RED 0.682f
#define TILE_TEXTURE2_GREEN 0.572f
#define TILE_TEXTURE2_BLUE 0.329f

#define DIEING_STONE1_COLOR_RED 0.15f
#define DIEING_STONE1_COLOR_GREEN 0.17f
#define DIEING_STONE1_COLOR_BLUE 0.20f

#define DIEING_STONE2_COLOR_RED 0.21f
#define DIEING_STONE2_COLOR_GREEN 0.23f
#define DIEING_STONE2_COLOR_BLUE 0.26f

Tile gTiles[NUMBER_OF_TILES];

static TextureObject gSkyTex;

static TextureObject gTileTexture1;
static TextureObject gTileCrackedTexture1;
static TextureObject gTileTexture2;
static TextureObject gTileCrackedTexture2;

void loadTiles(void)
{
	for (int tileIndex = 0; tileIndex < NUMBER_OF_TILES; tileIndex++)
	{
		gTiles[tileIndex].state = true;
		gTiles[tileIndex].recovery_timer = 0;
		gTiles[tileIndex].isDead = false;
		gTiles[tileIndex].cracked = false;
		gTiles[tileIndex].crackedTime = 0.0f;
		gTiles[tileIndex].coloredID = NO_CHARACTER;
		gTiles[tileIndex].colorTime = 0;
		gTiles[tileIndex].predictedColorID = NO_CHARACTER;
		gTiles[tileIndex].predictedColorTime = 0;
		
		int rowIndex = tileIndex / 8;
		int columnIndex = tileIndex % 8;
		
		gTiles[tileIndex].x = -7.0f + 2.0f * columnIndex;
		gTiles[tileIndex].y = 12.5f + 2.0f * rowIndex;
		gTiles[tileIndex].z = TILE_ALIVE_Z;
		
		if ((((tileIndex / 8) % 2) ^ (tileIndex % 2)) != 0)
		{
			gTiles[tileIndex].red = TILE_TEXTURE1_RED;
			gTiles[tileIndex].green = TILE_TEXTURE1_GREEN;
			gTiles[tileIndex].blue = TILE_TEXTURE1_BLUE;
		}
		else
		{
			gTiles[tileIndex].red = TILE_TEXTURE2_RED;
			gTiles[tileIndex].green = TILE_TEXTURE2_GREEN;
			gTiles[tileIndex].blue = TILE_TEXTURE2_BLUE;
		}
	}
}

void restoreDefaultTileColor(int tileIndex)
{
	if ((((tileIndex / 8) % 2) ^ (tileIndex % 2)) != 0)
	{
		gTiles[tileIndex].red = TILE_TEXTURE1_RED;
		gTiles[tileIndex].green = TILE_TEXTURE1_GREEN;
		gTiles[tileIndex].blue = TILE_TEXTURE1_BLUE;
	}
	else
	{
		gTiles[tileIndex].red = TILE_TEXTURE2_RED;
		gTiles[tileIndex].green = TILE_TEXTURE2_GREEN;
		gTiles[tileIndex].blue = TILE_TEXTURE2_BLUE;
	}
}

void setDieingTileColor(int tileIndex)
{
	if ((((tileIndex / 8) % 2) ^ (tileIndex % 2)) != 0)
	{
		gTiles[tileIndex].red = DIEING_STONE1_COLOR_RED;
		gTiles[tileIndex].green = DIEING_STONE1_COLOR_GREEN;
		gTiles[tileIndex].blue = DIEING_STONE1_COLOR_BLUE;
	}
	else
	{
		gTiles[tileIndex].red = DIEING_STONE2_COLOR_RED;
		gTiles[tileIndex].green = DIEING_STONE2_COLOR_GREEN;
		gTiles[tileIndex].blue = DIEING_STONE2_COLOR_BLUE;
	}
	
	gTiles[tileIndex].coloredID = DIEING_STONE_ID;
}

void _clearPredictedColor(int tileIndex)
{
	gTiles[tileIndex].predictedColorID = NO_CHARACTER;
	gTiles[tileIndex].predictedColorTime = 0;
	
	if (gTiles[tileIndex].coloredID == NO_CHARACTER)
	{
		restoreDefaultTileColor(tileIndex);
		
		gTiles[tileIndex].crackedTime = 0.0f;
		gTiles[tileIndex].cracked = false;
	}
}

void clearPredictedColor(int tileIndex)
{
	if (gTiles[tileIndex].predictedColorID != NO_CHARACTER)
	{
		_clearPredictedColor(tileIndex);
	}
}

void clearPredictedColorWithTime(int tileIndex, float currentTime)
{
	if ((currentTime - gTiles[tileIndex].predictedColorTime) >= 0.6f)
	{
		clearPredictedColor(tileIndex);
	}
}

void clearPredictedColorsForCharacter(int characterID)
{
	for (int tileIndex = 0; tileIndex < NUMBER_OF_TILES; tileIndex++)
	{
		if (gTiles[tileIndex].predictedColorID == characterID)
		{
			_clearPredictedColor(tileIndex);
		}
	}
}

int rightTileIndex(int tileIndex)
{
	int rightIndex = tileIndex + 1;
	if (rightIndex < 0 || rightIndex >= NUMBER_OF_TILES || rightIndex % 8 == 0)
	{
		return -1;
	}
	
	return rightIndex;
}

int leftTileIndex(int tileIndex)
{
	int leftIndex = tileIndex - 1;
	if (leftIndex < 0 || leftIndex >= NUMBER_OF_TILES || tileIndex % 8 == 0)
	{
		return -1;
	}
	
	return leftIndex;
}

int upTileIndex(int tileIndex)
{
	int upIndex = tileIndex + 8;
	if (upIndex < 0 || upIndex >= NUMBER_OF_TILES)
	{
		return -1;
	}
	
	return upIndex;
}

int downTileIndex(int tileIndex)
{
	int downIndex = tileIndex - 8;
	if (downIndex < 0 || downIndex >= NUMBER_OF_TILES)
	{
		return -1;
	}
	
	return downIndex;
}

// Tests if a tile is suitable for a spawning location
bool availableTileIndex(int tileIndex)
{
	int tile_loc = tileIndex;
	
	if (gTiles[tile_loc].state == false)
		return false;
	
	if (gTiles[tile_loc].z < TILE_ALIVE_Z)
		return false;
	
	if (gTiles[tile_loc].coloredID != NO_CHARACTER)
		return false;
	
	int column = tile_loc % 8;
	int row = tile_loc / 8;
	
	if (CHARACTER_IS_ALIVE(&gRedRover) && (columnOfCharacter(&gRedRover) == column || rowOfCharacter(&gRedRover) == row))
	{
		return false;
	}
	
	if (CHARACTER_IS_ALIVE(&gGreenTree) && (columnOfCharacter(&gGreenTree) == column || rowOfCharacter(&gGreenTree) == row))
	{
		return false;
	}
	
	if (CHARACTER_IS_ALIVE(&gPinkBubbleGum) && (columnOfCharacter(&gPinkBubbleGum) == column || rowOfCharacter(&gPinkBubbleGum) == row))
	{
		return false;
	}
	
	if (CHARACTER_IS_ALIVE(&gBlueLightning) && (columnOfCharacter(&gBlueLightning) == column || rowOfCharacter(&gBlueLightning) == row))
	{
		return false;
	}
	
	return true;
}

void loadSceneryTextures(Renderer *renderer)
{
	gSkyTex = loadTexture(renderer, "Data/Textures/sky.bmp");
	
	gTileTexture1 = loadTexture(renderer, "Data/Textures/tiletex.bmp");
	gTileCrackedTexture1 = loadTexture(renderer, "Data/Textures/tiletex_cracked.bmp");
	gTileTexture2 = loadTexture(renderer, "Data/Textures/tiletex2.bmp");
	gTileCrackedTexture2 = loadTexture(renderer, "Data/Textures/tiletex2_cracked.bmp");
}

void drawSky(Renderer *renderer, RendererOptions options)
{
	static BufferArrayObject vertexAndTextureArrayObject;
	static BufferObject indicesBufferObject;
	static bool initializedBuffers;
	
	if (!initializedBuffers)
	{
		ZGFloat vertexAndTextureCoordinates[] =
		{
			// vertices
			-16.0f, 16.0f, 0.0f, 1.0f,
			16.0f, 16.0f, 0.0f, 1.0f,
			16.0f, -16.0f, 0.0f, 1.0f,
			-16.0f, -16.0f, 0.0f, 1.0f,
			
			// texture coordinates
			0.0f, 0.0f,
			1.0f, 0.0f,
			1.0f, 1.0f,
			0.0f, 1.0f
		};
		
		vertexAndTextureArrayObject = createVertexAndTextureCoordinateArrayObject(renderer, vertexAndTextureCoordinates, 16 * sizeof(*vertexAndTextureCoordinates), 8 * sizeof(*vertexAndTextureCoordinates));
		indicesBufferObject = rectangleIndexBufferObject(renderer);
		
		initializedBuffers = true;
	}
	
	mat4_t modelViewMatrix = m4_mul(m4_translation((vec3_t){0.0f, 0.0f, -38.0f}), m4_scaling((vec3_t){computeProjectionAspectRatio(renderer), 1.0f, 1.0f}));
	
	drawTextureWithVerticesFromIndices(renderer, modelViewMatrix, gSkyTex, RENDERER_TRIANGLE_MODE, vertexAndTextureArrayObject, indicesBufferObject, 6, (color4_t){1.0f, 1.0f, 1.0f, 0.75f}, options);
}

void drawTiles(Renderer *renderer)
{
	static BufferArrayObject vertexAndTextureCoordinateArrayObject;
	static BufferObject indicesBufferObject;
	static bool initializedBuffers;
	
	if (!initializedBuffers)
	{
		ZGFloat verticesAndTextureCoordinates[] =
		{
			// Vertices
			
			// Bottom
			-1.0f, -1.0f, 1.0f, 1.0f,
			-1.0f, -1.0f, -1.0f, 1.0f,
			1.0f, -1.0f, -1.0f, 1.0f,
			1.0f, -1.0f, 1.0f, 1.0f,
			
			// Left
			-1.0f, 1.0f, 1.0f, 1.0f,
			-1.0f, 1.0f, -1.0f, 1.0f,
			-1.0f, -1.0f, -1.0f, 1.0f,
			-1.0f, -1.0f, 1.0f, 1.0f,
			
			// Right
			1.0f, 1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, -1.0f, 1.0f,
			1.0f, -1.0f, -1.0f, 1.0f,
			1.0f, -1.0f, 1.0f, 1.0f,
			
			// Front
			-1.0f, 1.0f, 1.0f, 1.0f,
			-1.0f, -1.0f, 1.0f, 1.0f,
			1.0f, -1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, 1.0f, 1.0f,
			
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
		
		vertexAndTextureCoordinateArrayObject = createVertexAndTextureCoordinateArrayObject(renderer, verticesAndTextureCoordinates, sizeof(*verticesAndTextureCoordinates) * 64, sizeof(*verticesAndTextureCoordinates) * 32);
		
		indicesBufferObject = createIndexBufferObject(renderer, indices, sizeof(indices));
		
		initializedBuffers = true;
	}
	
	mat4_t worldRotationMatrix = m4_rotation_x(-40.0f * ((ZGFloat)M_PI / 180.0f));
	
	for (int i = 0; i < NUMBER_OF_TILES; i++)
	{
		if (gTiles[i].z > TILE_TERMINATING_Z)
		{
			mat4_t modelTranslationMatrix = m4_translation((vec3_t){gTiles[i].x , gTiles[i].y, gTiles[i].z});
			mat4_t modelViewMatrix = m4_mul(worldRotationMatrix, modelTranslationMatrix);
			
			bool cracked = gTiles[i].cracked;
			
			TextureObject texture = (((i / 8) % 2) ^ (i % 2)) != 0 ? (!cracked ? gTileTexture1 : gTileCrackedTexture1) : (!cracked ? gTileTexture2 : gTileCrackedTexture2);
			
			drawTextureWithVerticesFromIndices(renderer, modelViewMatrix, texture, RENDERER_TRIANGLE_MODE, vertexAndTextureCoordinateArrayObject, indicesBufferObject, 24, (color4_t){gTiles[i].red, gTiles[i].green, gTiles[i].blue, 1.0f}, RENDERER_OPTION_NONE);
		}
	}
}
