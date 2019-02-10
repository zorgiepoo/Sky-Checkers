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

static TextureObject gSkyTex;

static TextureObject gTileTexture1;
static TextureObject gTileTexture2;

void loadTiles(void)
{
	for (int tileIndex = 0; tileIndex < NUMBER_OF_TILES; tileIndex++)
	{
		gTiles[tileIndex].state = SDL_TRUE;
		gTiles[tileIndex].recovery_timer = 0;
		gTiles[tileIndex].isDead = SDL_FALSE;
		gTiles[tileIndex].coloredID = NO_CHARACTER;
		gTiles[tileIndex].predictedColorID = NO_CHARACTER;
		gTiles[tileIndex].predictedColorTime = 0;
		
		int rowIndex = tileIndex / 8;
		int columnIndex = tileIndex % 8;
		
		gTiles[tileIndex].x = -7.0f + 2.0f * columnIndex;
		gTiles[tileIndex].y = 12.5f + 2.0f * rowIndex;
		gTiles[tileIndex].z = TILE_ALIVE_Z;
		
		if ((((tileIndex / 8) % 2) ^ (tileIndex % 2)) != 0)
		{
			gTiles[tileIndex].red = 0.8f;
			gTiles[tileIndex].green = 0.8f;
			gTiles[tileIndex].blue = 0.8f;
		}
		else
		{
			gTiles[tileIndex].red = 0.682f;
			gTiles[tileIndex].green = 0.572f;
			gTiles[tileIndex].blue = 0.329f;
		}
	}
}

void restoreDefaultTileColor(int tileIndex)
{
	if ((((tileIndex / 8) % 2) ^ (tileIndex % 2)) != 0)
	{
		gTiles[tileIndex].red = 0.8f;
		gTiles[tileIndex].green = 0.8f;
		gTiles[tileIndex].blue = 0.8f;
	}
	else
	{
		gTiles[tileIndex].red = 0.682f;
		gTiles[tileIndex].green = 0.572f;
		gTiles[tileIndex].blue = 0.329f;
	}
}

void clearPredictedColor(int tileIndex)
{
	if (gTiles[tileIndex].predictedColorID != NO_CHARACTER)
	{
		gTiles[tileIndex].predictedColorID = NO_CHARACTER;
		gTiles[tileIndex].predictedColorTime = 0;
		
		if (gTiles[tileIndex].coloredID == NO_CHARACTER)
		{
			restoreDefaultTileColor(tileIndex);
		}
	}
}

void clearPredictedColorWithTime(int tileIndex, uint32_t currentTime)
{
	if ((currentTime - gTiles[tileIndex].predictedColorTime) >= 600)
	{
		clearPredictedColor(tileIndex);
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
SDL_bool availableTileIndex(int tileIndex)
{
	int tile_loc = tileIndex;
	
	if (gTiles[tile_loc].state == SDL_FALSE)
		return SDL_FALSE;
	
	if (gTiles[tile_loc].z < TILE_ALIVE_Z)
		return SDL_FALSE;
	
	if (gTiles[tile_loc].coloredID != NO_CHARACTER)
		return SDL_FALSE;
	
	int column = tile_loc % 8;
	int row = tile_loc / 8;
	
	if (CHARACTER_IS_ALIVE(&gRedRover) && (columnOfCharacter(&gRedRover) == column || rowOfCharacter(&gRedRover) == row))
	{
		return SDL_FALSE;
	}
	
	if (CHARACTER_IS_ALIVE(&gGreenTree) && (columnOfCharacter(&gGreenTree) == column || rowOfCharacter(&gGreenTree) == row))
	{
		return SDL_FALSE;
	}
	
	if (CHARACTER_IS_ALIVE(&gPinkBubbleGum) && (columnOfCharacter(&gPinkBubbleGum) == column || rowOfCharacter(&gPinkBubbleGum) == row))
	{
		return SDL_FALSE;
	}
	
	if (CHARACTER_IS_ALIVE(&gBlueLightning) && (columnOfCharacter(&gBlueLightning) == column || rowOfCharacter(&gBlueLightning) == row))
	{
		return SDL_FALSE;
	}
	
	return SDL_TRUE;
}

void loadSceneryTextures(Renderer *renderer)
{
	gSkyTex = loadTexture(renderer, "Data/Textures/sky.bmp");
	
	gTileTexture1 = loadTexture(renderer, "Data/Textures/tiletex.bmp");
	gTileTexture2 = loadTexture(renderer, "Data/Textures/tiletex2.bmp");
}

void drawSky(Renderer *renderer, RendererOptions options)
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
	
	drawTextureWithVerticesFromIndices(renderer, modelViewMatrix, gSkyTex, RENDERER_TRIANGLE_MODE, vertexAndTextureArrayObject, indicesBufferObject, 6, (color4_t){1.0f, 1.0f, 1.0f, 0.9f}, options);
}

void drawTiles(Renderer *renderer)
{
	static BufferArrayObject vertexAndTextureCoordinateArrayObject;
	static BufferObject indicesBufferObject;
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
			1.0f, 1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
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
		
		initializedBuffers = SDL_TRUE;
	}
	
	mat4_t worldRotationMatrix = m4_rotation_x(-40.0f * ((float)M_PI / 180.0f));
	
	for (int i = 0; i < NUMBER_OF_TILES; i++)
	{
		if (gTiles[i].z > TILE_TERMINATING_Z)
		{
			mat4_t modelTranslationMatrix = m4_translation((vec3_t){gTiles[i].x , gTiles[i].y, gTiles[i].z});
			mat4_t modelViewMatrix = m4_mul(worldRotationMatrix, modelTranslationMatrix);
			
			TextureObject texture = (((i / 8) % 2) ^ (i % 2)) != 0 ? gTileTexture1 : gTileTexture2;
			
			drawTextureWithVerticesFromIndices(renderer, modelViewMatrix, texture, RENDERER_TRIANGLE_MODE, vertexAndTextureCoordinateArrayObject, indicesBufferObject, 24, (color4_t){gTiles[i].red, gTiles[i].green, gTiles[i].blue, 1.0f}, RENDERER_OPTION_NONE);
		}
	}
}
