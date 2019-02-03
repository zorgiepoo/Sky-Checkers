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

#include "characters.h"
#include "collision.h"
#include "font.h"
#include "network.h"
#include "utilities.h"

int gAIMode;
int gAINetMode = 5; // AI_EASY_MODE

int gNumberOfNetHumans = 1;

Character gRedRover;
Character gGreenTree;
Character gPinkBubbleGum;
Character gBlueLightning;

static TextureObject gCharacterTex;

static BufferArrayObject gCharacterVertexAndTextureCoordinateArrayObject;
static BufferObject gCharacterIndicesBufferObject;

static BufferArrayObject gIconVertexAndTextureCoordinateArrayObject;

static void randomizeCharacterDirection(Character *character);

/* Note: Does not initialize the character's weapon */
void loadCharacter(Character *character)
{	
	if (!gNetworkConnection || gNetworkConnection->type == NETWORK_SERVER_TYPE)
	{
		spawnCharacter(character);
		randomizeCharacterDirection(character);
	}
	else if (gNetworkConnection && gNetworkConnection->type == NETWORK_CLIENT_TYPE)
	{
		character->active = SDL_TRUE;
	}
	
	character->speed = INITIAL_CHARACTER_SPEED;
	character->recovery_timer = 0;
	character->animation_timer = 0;
	character->coloredTiles = SDL_FALSE;
	character->needTileLoc = SDL_TRUE;
	character->recovery_time_delay = INITIAL_RECOVERY_TIME_DELAY;
	character->player_loc = -1;
	character->destroyedTileIndex = -1;
	
	character->xDiscrepancy = 0.0f;
	character->yDiscrepancy = 0.0f;
	character->movementConsumedCounter = 0;
	character->predictedDirectionTime = 0;
	character->predictedDirection = NO_DIRECTION;
	
	character->ai_timer = 0;
	character->time_alive = 0;
	
	character->kills = 0;
}

void resetCharacterWins(void)
{
	gRedRover.wins = 0;
	gPinkBubbleGum.wins = 0;
	gGreenTree.wins = 0;
	gBlueLightning.wins = 0;
}

void initCharacters(void)
{
	// redRover
	loadCharacter(&gRedRover);
	gRedRover.weap = malloc(sizeof(Weapon));
	initWeapon(gRedRover.weap);
	gRedRover.weap->red = 1.0f;
	gRedRover.weap->green = 0.0f;
	gRedRover.weap->blue = 0.0f;
	gRedRover.netState = NETWORK_NO_STATE;
	gRedRover.netName = NULL;
	gRedRover.backup_state = 0;
	gRedRover.red = 0.9f;
	gRedRover.green = 0.0f;
	gRedRover.blue = 0.0f;
	
	// greenTree
	loadCharacter(&gGreenTree);
	gGreenTree.weap = malloc(sizeof(Weapon));
	initWeapon(gGreenTree.weap);
	gGreenTree.weap->red = 0.2196f;
	gGreenTree.weap->green = 0.851f;
	gGreenTree.weap->blue = 0.2623f;
	gGreenTree.netState = NETWORK_NO_STATE;
	gGreenTree.netName = NULL;
	gGreenTree.backup_state = 0;
	gGreenTree.red = 0.3f;
	gGreenTree.green = 1.0f;
	gGreenTree.blue = 0.3f;
	
	// pinkBubbleGum
	loadCharacter(&gPinkBubbleGum);
	gPinkBubbleGum.weap = malloc(sizeof(Weapon));
	initWeapon(gPinkBubbleGum.weap);
	gPinkBubbleGum.weap->red = 0.988f;
	gPinkBubbleGum.weap->green = 0.486f;
	gPinkBubbleGum.weap->blue = 0.796f;
	gPinkBubbleGum.netState = NETWORK_NO_STATE;
	gPinkBubbleGum.netName = NULL;
	gPinkBubbleGum.backup_state = 0;
	gPinkBubbleGum.red = 1.0f;
	gPinkBubbleGum.green = 0.6f;
	gPinkBubbleGum.blue = 0.6f;
	
	// blueLightning
	loadCharacter(&gBlueLightning);
	gBlueLightning.weap = malloc(sizeof(Weapon));
	initWeapon(gBlueLightning.weap);
	gBlueLightning.weap->red = 0.09f;
	gBlueLightning.weap->green = 0.4157f;
	gBlueLightning.weap->blue = 0.5019f;
	gBlueLightning.netState = NETWORK_NO_STATE;
	gBlueLightning.netName = NULL;
	gBlueLightning.backup_state = 0;
	gBlueLightning.red = 0.3f;
	gBlueLightning.green = 0.5f;
	gBlueLightning.blue = 0.6f;
	
	resetCharacterWins();
}

static void restoreCharacterBackupState(Character *character)
{
	if (character->backup_state)
	{
		character->state = character->backup_state;
	}
	// just in case something goes wrong
	else if (!character->state)
	{
		character->state = CHARACTER_AI_STATE;
	}
	
	character->backup_state = 0;
}

void restoreAllBackupStates(void)
{
	restoreCharacterBackupState(&gPinkBubbleGum);
	restoreCharacterBackupState(&gRedRover);
	restoreCharacterBackupState(&gGreenTree);
	restoreCharacterBackupState(&gBlueLightning);
}

int offlineCharacterState(Character *character)
{
	return (character->backup_state ? character->backup_state : character->state);
}

void loadCharacterTextures(Renderer *renderer)
{
	gCharacterTex = loadTexture(renderer, "Data/Textures/face.bmp");
}

// http://www.songho.ca/opengl/gl_sphere.html
static void buildSphere(float *vertices, float *textureCoordinates, unsigned short *indices, int stackCount, int sectorCount, float radius)
{
	float stackStep = (float)M_PI / stackCount;
	float sectorStep = 2 * (float)M_PI / sectorCount;
	
	int vertexIndex = 0;
	int textureCoordinateIndex = 0;
	
	for (int stackIndex = 0; stackIndex <= stackCount; stackIndex++)
	{
		float stackAngle = ((float)M_PI / 2.0f) - stackIndex * stackStep;
		float xy = radius * cosf(stackAngle);
		float z = radius * sinf(stackAngle);
		
		for (int sectorIndex = 0; sectorIndex <= sectorCount; sectorIndex++)
		{
			float sectorAngle = sectorIndex * sectorStep;
			
			float x = xy * cosf(sectorAngle);
			float y = xy * sinf(sectorAngle);
			
			vertices[vertexIndex] = x;
			vertexIndex++;
			vertices[vertexIndex] = y;
			vertexIndex++;
			vertices[vertexIndex] = z;
			vertexIndex++;
			
			float s = (float)sectorIndex / sectorCount;
			float t = (float)stackIndex / stackCount;
			
			textureCoordinates[textureCoordinateIndex] = s;
			textureCoordinateIndex++;
			textureCoordinates[textureCoordinateIndex] = t;
			textureCoordinateIndex++;
		}
	}
	
	unsigned short indiceIndex = 0;
	
	for (int stackIndex = 0; stackIndex < stackCount; stackIndex++)
	{
		unsigned short k1 = stackIndex * (sectorCount + 1);
		unsigned short k2 = k1 + sectorCount + 1;
		
		for (int sectorIndex = 0; sectorIndex < sectorCount; sectorIndex++)
		{
			if (stackIndex != 0)
			{
				indices[indiceIndex] = k1;
				indiceIndex++;
				indices[indiceIndex] = k2;
				indiceIndex++;
				indices[indiceIndex] = k1 + 1;
				indiceIndex++;
			}
			
			if (stackIndex != (stackCount - 1))
			{
				indices[indiceIndex] = k1 + 1;
				indiceIndex++;
				indices[indiceIndex] = k2;
				indiceIndex++;
				indices[indiceIndex] = k2 + 1;
				indiceIndex++;
			}
			
			k1++;
			k2++;
		}
	}
}

// https://stackoverflow.com/questions/27238793/texture-mapping-a-circle
static void buildCircle(float *vertices, float *textureCoordinates, float radius, int numberOfPoints)
{
	float theta = 2 * (float)M_PI / (float)numberOfPoints;
	float cosTheta = cosf(theta);
	float sinTheta = sinf(theta);
	
	int vertexIndex = 0;
	int textureCoordinateIndex = 0;
	
	vertices[vertexIndex] = 0.0f;
	vertexIndex++;
	vertices[vertexIndex] = 0.0f;
	vertexIndex++;
	vertices[vertexIndex] = 0.0f;
	vertexIndex++;
	
	textureCoordinates[textureCoordinateIndex] = 0.5f;
	textureCoordinateIndex++;
	textureCoordinates[textureCoordinateIndex] = 0.5f;
	textureCoordinateIndex++;
	
	float x = radius;
	float y = 0.0f;
	
	for (int pointIndex = 0; pointIndex <= numberOfPoints; pointIndex++)
	{
		vertices[vertexIndex] = x;
		vertexIndex++;
		vertices[vertexIndex] = y;
		vertexIndex++;
		vertices[vertexIndex] = 0.0f;
		vertexIndex++;
		
		float textureX = (x / radius + 1.0f) * 0.5f;
		float textureY = (y / radius + 1.0f) * 0.5f;
		
		textureCoordinates[textureCoordinateIndex] = textureX;
		textureCoordinateIndex++;
		textureCoordinates[textureCoordinateIndex] = textureY;
		textureCoordinateIndex++;
		
		if ((pointIndex + 1) % 2 == 0)
		{
			vertices[vertexIndex] = 0.0f;
			vertexIndex++;
			vertices[vertexIndex] = 0.0f;
			vertexIndex++;
			vertices[vertexIndex] = 0.0f;
			vertexIndex++;
			
			textureCoordinates[textureCoordinateIndex] = 0.5f;
			textureCoordinateIndex++;
			textureCoordinates[textureCoordinateIndex] = 0.5f;
			textureCoordinateIndex++;
		}
		
		float lastX = x;
		x = cosTheta * x - sinTheta * y;
		y = sinTheta * lastX + cosTheta * y;
	}
}

void buildCharacterModels(Renderer *renderer)
{
	// Build character model
	float *characterVerticesAndTextureCoordinates;
	size_t characterVerticesSize = sizeof(*characterVerticesAndTextureCoordinates) * 2883;
	size_t characterTextureCoordinatesSize = sizeof(*characterVerticesAndTextureCoordinates) * 1922;
	
	characterVerticesAndTextureCoordinates = malloc(characterVerticesSize + characterTextureCoordinatesSize);
	
	uint16_t *characterIndices;
	size_t characterIndicesSize = sizeof(*characterIndices) * 5220;
	characterIndices = malloc(characterIndicesSize);
	
	buildSphere(characterVerticesAndTextureCoordinates, characterVerticesAndTextureCoordinates + characterVerticesSize / sizeof(*characterVerticesAndTextureCoordinates), characterIndices, 30, 30, 0.6f);
	
	gCharacterVertexAndTextureCoordinateArrayObject = createVertexAndTextureCoordinateArrayObject(renderer, characterVerticesAndTextureCoordinates, characterVerticesSize, characterTextureCoordinatesSize);
	
	gCharacterIndicesBufferObject = createBufferObject(renderer, characterIndices, characterIndicesSize);
	
	free(characterVerticesAndTextureCoordinates);
	free(characterIndices);
	
	// Build character icon model
	float *iconVerticesAndTextureCoordinates;
	size_t iconVerticesSize = sizeof(*iconVerticesAndTextureCoordinates) * 1806;
	size_t iconTextureCoordinatesSize = sizeof(*iconVerticesAndTextureCoordinates) * 1204;
	
	iconVerticesAndTextureCoordinates = malloc(iconVerticesSize + iconTextureCoordinatesSize);
	
	buildCircle(iconVerticesAndTextureCoordinates, iconVerticesAndTextureCoordinates + iconVerticesSize / sizeof(*iconVerticesAndTextureCoordinates), 0.4f, 400);
	
	gIconVertexAndTextureCoordinateArrayObject = createVertexAndTextureCoordinateArrayObject(renderer, iconVerticesAndTextureCoordinates, iconVerticesSize, iconTextureCoordinatesSize);
	
	free(iconVerticesAndTextureCoordinates);
}

static float zRotationForCharacter(Character *character)
{
	switch (character->pointing_direction)
	{
		case RIGHT:
			return (float)M_PI;
		case LEFT:
			return 0.0f;
		case DOWN:
			return 100.0f * ((float)M_PI / 180.0f);
		case UP:
			return 3.0f * (float)M_PI / 2.0f;
	}
	return 0.0f;
}

static mat4_t modelViewMatrixForCharacter(Character *character, mat4_t worldMatrix)
{
	mat4_t modelTranslationMatrix = m4_translation((vec3_t){character->x, character->y, character->z});
	mat4_t modelRotationMatrix = m4_rotation_z(zRotationForCharacter(character));
	
	return m4_mul(m4_mul(worldMatrix, modelTranslationMatrix), modelRotationMatrix);
}

static void drawCharacter(Renderer *renderer, Character *character, mat4_t worldMatrix)
{
	// don't draw the character if they're not in the scene
	if (character->z > CHARACTER_TERMINATING_Z)
	{
		mat4_t modelViewMatrix = modelViewMatrixForCharacter(character, worldMatrix);
		
		drawTextureWithVerticesFromIndices(renderer, modelViewMatrix, gCharacterTex, RENDERER_TRIANGLE_MODE, gCharacterVertexAndTextureCoordinateArrayObject, gCharacterIndicesBufferObject, 5220, (color4_t){character->red, character->green, character->blue, 1.0f}, RENDERER_OPTION_NONE);
	}
}

void drawCharacters(Renderer *renderer)
{
	mat4_t worldRotationMatrix = m4_rotation_x(-40.0f * ((float)M_PI / 180.0f));
	mat4_t worldTranslationMatrix = m4_translation((vec3_t){-7.0f, 12.5f, -25.0f});
	mat4_t worldMatrix = m4_mul(worldRotationMatrix, worldTranslationMatrix);
	
	drawCharacter(renderer, &gRedRover, worldMatrix);
	drawCharacter(renderer, &gGreenTree, worldMatrix);
	drawCharacter(renderer, &gPinkBubbleGum, worldMatrix);
	drawCharacter(renderer, &gBlueLightning, worldMatrix);
}

static mat4_t characterIconModelViewMatrix(mat4_t modelViewMatrix)
{
	return m4_mul(modelViewMatrix, m4_rotation_x((float)M_PI));
}

static void drawCharacterIcon(Renderer *renderer, mat4_t modelViewMatrix, Character *character)
{
	drawTextureWithVertices(renderer, characterIconModelViewMatrix(modelViewMatrix), gCharacterTex, RENDERER_TRIANGLE_STRIP_MODE, gIconVertexAndTextureCoordinateArrayObject, 1204 / 2, (color4_t){character->red, character->green, character->blue, 1.0f}, RENDERER_OPTION_NONE);
}

void drawCharacterIcons(Renderer *renderer, const mat4_t *translations)
{
	drawCharacterIcon(renderer, translations[0], &gPinkBubbleGum);
	drawCharacterIcon(renderer, translations[1], &gRedRover);
	drawCharacterIcon(renderer, translations[2], &gGreenTree);
	drawCharacterIcon(renderer, translations[3], &gBlueLightning);
}

static const char *labelForCharacter(Character *character, const char *playerNumberString)
{
	if (gNetworkConnection)
	{
		if (character->netName)
		{
			return character->netName;
		}
		// this is a clever way to see if this character is going to be an AI or a networked player
		else if ((gNetworkConnection->type == NETWORK_SERVER_TYPE && character->netState == NETWORK_PLAYING_STATE) ||
				 (gNetworkConnection->type == NETWORK_CLIENT_TYPE && gPinkBubbleGum.netState == NETWORK_PLAYING_STATE))
		{
			return "[AI]";
		}
	}
	else
	{
		if (character->state == CHARACTER_AI_STATE)
		{
			return "[AI]";
		}
		else
		{
			return playerNumberString;
		}
	}
	return playerNumberString;
}

static mat4_t playerLabelModelViewMatrix(mat4_t modelViewMatrix)
{
	return m4_mul(modelViewMatrix, m4_translation((vec3_t){0.5f, 0.0f, 0.0f}));
}

static void drawCharacterLives(Renderer *renderer, mat4_t modelViewMatrix, color4_t color, Character *character, float livesWidth, float livesHeight, const char *playerNumberString, float playerLabelWidth, float playerLabelHeight)
{
	if (character->lives != 0)
	{
		char buffer[256] = {0};
		snprintf(buffer, sizeof(buffer) - 1, "%d", character->lives);
		drawStringScaled(renderer, modelViewMatrix, color, 0.0035f, buffer);
	}
	
	const char *playerLabel = labelForCharacter(character, playerNumberString);
	drawStringLeftAligned(renderer, playerLabelModelViewMatrix(modelViewMatrix), color, 0.0027f, playerLabel);
}

#define LIVES_DRAWING_OFFSET 0.8f
void drawAllCharacterLives(Renderer *renderer, const mat4_t *iconTranslations)
{
	const mat4_t pinkBubbleGumModelViewMatrix = m4_mul(iconTranslations[0], m4_translation((vec3_t){LIVES_DRAWING_OFFSET, 0.0f, 0.0f}));
	const mat4_t redRoverModelViewMatrix = m4_mul(iconTranslations[1], m4_translation((vec3_t){LIVES_DRAWING_OFFSET, 0.0f, 0.0f}));
	const mat4_t greenTreeModelViewMatrix = m4_mul(iconTranslations[2], m4_translation((vec3_t){LIVES_DRAWING_OFFSET, 0.0f, 0.0f}));
	const mat4_t blueLightningModelViewMatrix = m4_mul(iconTranslations[3], m4_translation((vec3_t){LIVES_DRAWING_OFFSET, 0.0f, 0.0f}));
	
	const color4_t pinkBubbleGumColor = (color4_t){1.0f, 0.6f, 0.6f, 1.0f};
	const color4_t redRoverColor = (color4_t){0.9f, 0.0f, 0.0f, 1.0f};
	const color4_t greenTreeColor = (color4_t){0.0f, 1.0f, 0.0f, 1.0f};
	const color4_t blueLightningColor = (color4_t){0.0f, 0.0f, 1.0f, 1.0f};
	
	const float playerLabelWidth = 1.0f / 1.52f;
	const float playerLabelHeight = 0.5f / 1.52f;
	
	const float livesWidth = 0.5f / 1.52f;
	const float livesHeight = 0.5f / 1.52f;
	
	drawCharacterLives(renderer, pinkBubbleGumModelViewMatrix, pinkBubbleGumColor, &gPinkBubbleGum, livesWidth, livesHeight, "[P1]", playerLabelWidth, playerLabelHeight);
	drawCharacterLives(renderer, redRoverModelViewMatrix, redRoverColor, &gRedRover, livesWidth, livesHeight, "[P2]", playerLabelWidth, playerLabelHeight);
	drawCharacterLives(renderer, greenTreeModelViewMatrix, greenTreeColor, &gGreenTree, livesWidth, livesHeight, "[P3]", playerLabelWidth, playerLabelHeight);
	drawCharacterLives(renderer, blueLightningModelViewMatrix, blueLightningColor, &gBlueLightning, livesWidth, livesHeight, "[P4]", playerLabelWidth, playerLabelHeight);
}

/*
 * Retrieves and stores character pointers (which won't be the same as characterA) into characterB, characterC, and characterD
 */
void getOtherCharacters(Character *characterA, Character **characterB, Character **characterC, Character **characterD)
{	
	if (characterA == &gRedRover)
	{
		*characterB = &gPinkBubbleGum;
		*characterC = &gGreenTree;
		*characterD = &gBlueLightning;
	}
	else if (characterA == &gPinkBubbleGum)
	{
		*characterB = &gRedRover;
		*characterC = &gGreenTree;
		*characterD = &gBlueLightning;
	}
	else if (characterA == &gBlueLightning)
	{
		*characterB = &gRedRover;
		*characterC = &gGreenTree;
		*characterD = &gPinkBubbleGum;
	}
	else if (characterA == &gGreenTree)
	{
		*characterB = &gRedRover;
		*characterC = &gPinkBubbleGum;
		*characterD = &gBlueLightning;
	}
}

int IDOfCharacter(Character *character)
{
	if (character == &gRedRover)
	{
		return RED_ROVER;
	}
	else if (character == &gGreenTree)
	{
		return GREEN_TREE;
	}
	else if (character == &gBlueLightning)
	{
		return BLUE_LIGHTNING;
	}
	
	return PINK_BUBBLE_GUM;
}

Character *getCharacter(int ID)
{
	if (ID == RED_ROVER)
	{
		return &gRedRover;
	}
	
	if (ID == GREEN_TREE)
	{
		return &gGreenTree;
	}
	
	if (ID == BLUE_LIGHTNING)
	{
		return &gBlueLightning;
	}
	
	if (ID == PINK_BUBBLE_GUM)
	{
		return &gPinkBubbleGum;
	}
	
	return NULL;
}

void spawnCharacter(Character *character)
{
	int randOne;
	int randTwo;
	SDL_bool isFree = SDL_FALSE;
	
	while (!isFree)
	{
		randOne = (mt_random() % 15);
		randTwo = (mt_random() % 15);
		
		isFree = availableTile((float)randOne, (float)randTwo);
	}
	
	character->x = (float)randOne;
	character->y = (float)randTwo;
	character->z = CHARACTER_ALIVE_Z;
	character->active = SDL_TRUE;
	character->direction = NO_DIRECTION;
}

static void randomizeCharacterDirection(Character *character)
{
	int newDirection = (mt_random() % 4) + 1;
	turnCharacter(character, newDirection);
}

static void sendCharacterMovement(Character *character)
{
	if (gNetworkConnection)
	{
		if (gNetworkConnection->type == NETWORK_SERVER_TYPE)
		{
			GameMessage message;
			message.type = CHARACTER_MOVED_UPDATE_MESSAGE_TYPE;
			message.movedUpdate.characterID = IDOfCharacter(character);
			message.movedUpdate.x = character->x;
			message.movedUpdate.y = character->y;
			message.movedUpdate.dead = !CHARACTER_IS_ALIVE(character);
			message.movedUpdate.direction = character->direction;
			message.movedUpdate.pointing_direction = character->pointing_direction;
			
			sendToClients(0, &message);
		}
	}
}

void moveCharacter(Character *character, double timeDelta)
{
	if (character->active)
	{
		Character *characterB = NULL;
		Character *characterC = NULL;
		Character *characterD = NULL;
		
		int direction = character->direction;
		
		turnCharacter(character, direction);
		
		if (direction != NO_DIRECTION)
		{
			getOtherCharacters(character, &characterB, &characterC, &characterD);
		}
		
		// Test which direction the character is moving in, then move the character in that direction if it won't collide with anything
		if (direction == RIGHT)
		{
			if (characterCanMove(direction, character) && checkCharacterCollision(direction, character, characterB, characterC, characterD))
			{
				character->x += character->speed * (float)timeDelta;
			}
		}
		else if (direction == LEFT)
		{
			if (characterCanMove(direction, character) && checkCharacterCollision(direction, character, characterB, characterC, characterD))
			{
				character->x -= character->speed * (float)timeDelta;
			}
		}
		else if (direction == DOWN)
		{
			if (characterCanMove(direction, character) && checkCharacterCollision(direction, character, characterB, characterC, characterD))
			{
				character->y -= character->speed * (float)timeDelta;
			}
		}
		else if (direction == UP)
		{
			if (characterCanMove(direction, character) && checkCharacterCollision(direction, character, characterB, characterC, characterD))
			{
				character->y += character->speed * (float)timeDelta;
			}
		}
	}
	
	sendCharacterMovement(character);
}

void turnCharacter(Character *character, int direction)
{
	if (!character->active)
	{
		return;
	}
	
	if (direction != NO_DIRECTION)
	{
		character->pointing_direction = direction;
	}
}

void fireCharacterWeapon(Character *character)
{
	if (gGameHasStarted)
	{
		if ((!character->weap->animationState || (gNetworkConnection && gNetworkConnection->type == NETWORK_CLIENT_TYPE)) && character->weap->fired && CHARACTER_IS_ALIVE(character))
		{
			int tileIndex = getTileIndexLocation((int)character->x, (int)character->y);
			if (tileIndex >= 0 && tileIndex < NUMBER_OF_TILES && gTiles[tileIndex].z == TILE_ALIVE_Z)
			{
				if (gNetworkConnection)
				{
					if (gNetworkConnection->type == NETWORK_SERVER_TYPE)
					{
						GameMessage message;
						message.type = CHARACTER_FIRED_UPDATE_MESSAGE_TYPE;
						message.firedUpdate.x = character->x;
						message.firedUpdate.y = character->y;
						message.firedUpdate.characterID = IDOfCharacter(character);
						message.firedUpdate.direction = character->pointing_direction;
						
						sendToClients(message.firedUpdate.characterID, &message);
					}
					else if (gNetworkConnection->type == NETWORK_CLIENT_TYPE && gNetworkConnection->character == character)
					{
						GameMessage message;
						message.type = CHARACTER_FIRED_REQUEST_MESSAGE_TYPE;
						
						sendToServer(message);
					}
				}
				
				// make sure character can't move while firing the weapon
				character->active = SDL_FALSE;
				
				character->weap->drawingState = SDL_TRUE;
				character->weap->animationState = SDL_TRUE;
				character->animation_timer = 0;
			}
		}
	}
	character->weap->fired = SDL_FALSE;
}

void prepareFiringCharacterWeapon(Character *character, float x, float y, float compensation)
{
	// don't bind the z value
	character->weap->x = x;
	character->weap->y = y;
	character->weap->initialX = x;
	character->weap->initialY = y;
	character->weap->compensation = compensation;
	character->weap->direction = character->pointing_direction;
	
	character->weap->fired = SDL_TRUE;
}
