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

const int NO_DIRECTION =				0;
const int RIGHT =						1;
const int LEFT =						2;
const int UP =							3;
const int DOWN =						4;

const int NO_CHARACTER =				0;
const int RED_ROVER =					1;
const int GREEN_TREE =					2;
const int BLUE_LIGHTNING =				3;
const int PINK_BUBBLE_GUM =				4;
const int WEAPON =						5;

static const float CHARACTER_SPEED =	0.08f;

const int CHARACTER_HUMAN_STATE =		1;
const int CHARACTER_AI_STATE =			2;

const int AI_EASY_MODE =				5;
const int AI_MEDIUM_MODE =				3;
const int AI_HARD_MODE =				1;

const int NETWORK_NO_STATE =			0;
const int NETWORK_PENDING_STATE =		5;
const int NETWORK_PLAYING_STATE =		6;

int gAIMode;
int gAINetMode = 5; // AI_EASY_MODE

int gNumberOfNetHumans = 1;

Character gRedRover;
Character gGreenTree;
Character gPinkBubbleGum;
Character gBlueLightning;

static GLuint gCharacterTex;

static GLfloat *gCharacterVertices;
static GLfloat *gCharacterTextureCoordinates;
static GLushort *gCharacterIndices;

static GLfloat *gIconVertices;
static GLfloat *gIconTextureCoordinates;

static void randomizeCharacterDirection(Character *character);

/* Note: Does not initialize the character's weapon */
void loadCharacter(Character *character)
{	
	if (!gNetworkConnection || gNetworkConnection->type != NETWORK_CLIENT_TYPE)
	{
		spawnCharacter(character);
		randomizeCharacterDirection(character);
		
		if (gNetworkConnection && gNetworkConnection->type == NETWORK_SERVER_TYPE)
		{
			// update movement
			sendToClients(0, "mo%i %f %f %f %i", IDOfCharacter(character), character->x, character->y, character->z, character->direction);
		}
	}
	else if (gNetworkConnection && gNetworkConnection->type == NETWORK_CLIENT_TYPE)
	{
		if (character->direction == NO_DIRECTION)
		{
			randomizeCharacterDirection(character);
		}
		character->z = 2.0;
	}
	
	character->recovery_timer = 0;
	character->animation_timer = 0;
	character->coloredTiles = SDL_FALSE;
	character->needTileLoc = SDL_TRUE;
	character->recovery_time_delay = 71;
	character->player_loc = 0;
	character->destroyed_tile = NULL;
	
	character->ai_timer = 0;
	character->time_alive = 0;
	
	character->kills = 0;
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
	loadTexture(renderer, "Data/Textures/face.bmp", &gCharacterTex);
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
			
			textureCoordinates[textureCoordinateIndex] = 0.5f;
			textureCoordinateIndex++;
			textureCoordinates[textureCoordinateIndex] = 0.5f;
			textureCoordinateIndex++;
		}
		
		float lastX = x;
		x = cosTheta * x - sinTheta * y;
		y = sinTheta * lastX + cosTheta * y;
	}
	
	//printf("icon info: %d, %d\n", vertexIndex, textureCoordinateIndex);
}

void buildCharacterModels(void)
{
	gCharacterVertices = malloc(sizeof(*gCharacterVertices) * 2883);
	gCharacterTextureCoordinates = malloc(sizeof(*gCharacterTextureCoordinates) * 1922);
	gCharacterIndices = malloc(sizeof(*gCharacterIndices) * 5220);
	
	buildSphere(gCharacterVertices, gCharacterTextureCoordinates, gCharacterIndices, 30, 30, 0.6f);
	
	gIconVertices = malloc(sizeof(*gIconVertices) * 1204);
	gIconTextureCoordinates = malloc(sizeof(*gIconTextureCoordinates) * 1204);
	
	buildCircle(gIconVertices, gIconTextureCoordinates, 0.4f, 400);
}

void drawCharacter(Renderer *renderer, Character *character)
{
	// don't draw the character if he's not on in the scene
	if (character->z <= -170.0)
		return;
	
	glEnable(GL_TEXTURE_2D);
	
	mat4_t worldRotationMatrix = m4_rotation_x(-40.0f * (M_PI / 180.0f));
	mat4_t worldTranslationMatrix = m4_translation((vec3_t){-7.0f, 12.5f, -25.0f});
	mat4_t modelTranslationMatrix = m4_translation((vec3_t){character->x, character->y, character->z});
	mat4_t modelRotationMatrix = m4_rotation_z(character->zRot);
	
	mat4_t modelViewMatrix = m4_mul(m4_mul(m4_mul(worldRotationMatrix, worldTranslationMatrix), modelTranslationMatrix), modelRotationMatrix);
	glLoadMatrixf(&modelViewMatrix.m00);
	
	glColor3f(character->red, character->green, character->blue);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glBindTexture(GL_TEXTURE_2D, gCharacterTex);
	
	glVertexPointer(3, GL_FLOAT, 0, gCharacterVertices);
	glTexCoordPointer(2, GL_FLOAT, 0, gCharacterTextureCoordinates);
	
	glDrawElements(GL_TRIANGLES, 5220, GL_UNSIGNED_SHORT, gCharacterIndices);
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glDisable(GL_TEXTURE_2D);
}

void drawCharacterIcon(Renderer *renderer, mat4_t modelViewMatrix, Character *character)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glEnable(GL_TEXTURE_2D);
	
	glColor4f(character->red, character->green, character->blue, 0.7f);
	
	mat4_t rotationMatrix = m4_rotation_x(M_PI);
	mat4_t rotatedIconModelViewMatrix = m4_mul(modelViewMatrix, rotationMatrix);
	
	glLoadMatrixf(&rotatedIconModelViewMatrix.m00);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glBindTexture(GL_TEXTURE_2D, gCharacterTex);
	
	glVertexPointer(2, GL_FLOAT, 0, gIconVertices);
	glTexCoordPointer(2, GL_FLOAT, 0, gIconTextureCoordinates);
	
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 1204 / 2);
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glDisable(GL_TEXTURE_2D);
	
	glDisable(GL_BLEND);
}

static void translateAndDrawCharacterIcon(Renderer *renderer, Character *character, float x, float y, float z)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){x, y, z});
	drawCharacterIcon(renderer, modelViewMatrix, character);
}

void drawCharacterIcons(Renderer *renderer)
{
	translateAndDrawCharacterIcon(renderer, &gPinkBubbleGum, -9.0f, -9.5f, -25.0f);
	translateAndDrawCharacterIcon(renderer, &gRedRover, -3.67f, -9.5f, -25.0f);
	translateAndDrawCharacterIcon(renderer, &gGreenTree, 1.63f, -9.5f, -25.0f);
	translateAndDrawCharacterIcon(renderer, &gBlueLightning, 6.93f, -9.5f, -25.0f);
}

static void drawCharacterLive(Renderer *renderer, mat4_t modelViewMatrix, color4_t color, Character *character, const char *playerNumberString)
{
	if (character->lives != 0)
	{
		drawStringf(renderer, modelViewMatrix, color, 0.5, 0.5, "%i", character->lives);
	}
	
	mat4_t playerDescriptionMatrix = m4_mul(modelViewMatrix, m4_translation((vec3_t){2.0f, 0.0f, 0.0f}));
	
	if (gNetworkConnection)
	{
		if (character->netName)
		{
			drawString(renderer, playerDescriptionMatrix, color, 1.0, 0.5, character->netName);
		}
		// this is a clever way to see if this character is going to be an AI or a networked player
		else if ((gNetworkConnection->type == NETWORK_SERVER_TYPE && character->netState == NETWORK_PLAYING_STATE) ||
				 (gNetworkConnection->type == NETWORK_CLIENT_TYPE && gPinkBubbleGum.netState == NETWORK_PLAYING_STATE))
		{
			drawString(renderer, playerDescriptionMatrix, color, 1.0, 0.5, "[AI]");
		}
	}
	else
	{
		if (character->state == CHARACTER_AI_STATE)
		{
			drawString(renderer, playerDescriptionMatrix, color, 1.0, 0.5, "[AI]");
		}
		else
		{
			drawString(renderer, playerDescriptionMatrix, color, 1.0, 0.5, playerNumberString);
		}
	}
}

void drawCharacterLives(Renderer *renderer)
{	
	// PinkBubbleGum
	drawCharacterLive(renderer, m4_translation((vec3_t){-12.3f, -14.4f, -38.0f}), (color4_t){1.0f, 0.6f, 0.6f, 1.0f}, &gPinkBubbleGum, "[P1]");
	
	// RedRover
	drawCharacterLive(renderer, m4_translation((vec3_t){-4.2f, -14.4f, -38.0f}), (color4_t){0.9f, 0.0f, 0.0f, 1.0f}, &gRedRover, "[P2]");
	
	// GreenTree
	drawCharacterLive(renderer, m4_translation((vec3_t){3.9f, -14.4f, -38.0f}), (color4_t){0.0f, 1.0f, 0.0f, 1.0f}, &gGreenTree, "[P3]");
	
	// BlueLightning
	drawCharacterLive(renderer, m4_translation((vec3_t){12.0f, -14.4f, -38.0f}), (color4_t){0.0f, 0.0f, 1.0f, 1.0f}, &gBlueLightning, "[P4]");
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
		
		isFree = availableTile((GLfloat)randOne, (GLfloat)randTwo);
	}
	
	character->x = (GLfloat)randOne;
	character->y = (GLfloat)randTwo;
	character->z = 2.0;
}

static void randomizeCharacterDirection(Character *character)
{
	character->direction = (mt_random() % 4) + 1;
	
	if (character->direction == RIGHT)
	{
		character->zRot = M_PI;
	}
	else if (character->direction == LEFT)
	{
		character->zRot = 0.0;
	}
	else if (character->direction == DOWN)
	{
		character->zRot = 100.0 * (M_PI / 180.0f);
	}
	else if (character->direction == UP)
	{
		character->zRot = 3.0f * M_PI / 2.0f;
	}
}

static void sendCharacterMovement(Character *character)
{
	if (gNetworkConnection)
	{
		if (gNetworkConnection->type == NETWORK_SERVER_TYPE)
		{
			sendToClients(0, "mo%i %f %f %f %i", IDOfCharacter(character), character->x, character->y, character->z, character->direction);
		}
		else if (gNetworkConnection->type == NETWORK_CLIENT_TYPE && character == gNetworkConnection->input->character)
		{
			char buffer[256];
			sprintf(buffer, "rm%i %i", IDOfCharacter(character), character->direction);
			
			sendto(gNetworkConnection->socket, buffer, strlen(buffer), 0, (struct sockaddr *)&gNetworkConnection->hostAddress, sizeof(gNetworkConnection->hostAddress));
		}
	}
}

void moveCharacter(Character *character, int direction)
{
	if (character->direction == NO_DIRECTION || !character->lives)
		return;
	
	Character *characterB = NULL;
	Character *characterC = NULL;
	Character *characterD = NULL;
	
	getOtherCharacters(character, &characterB, &characterC, &characterD);
	
	character->direction = direction;
	
	// Test which direction the character is moving in, then make the character look into that direction, and move the character in that direction if it won't collide with anything
	
	if (direction == RIGHT)
	{
		if (characterIsOutOfBounds(direction, character) && checkCharacterCollision(direction, character, characterB, characterC, characterD))
		{
			character->x += CHARACTER_SPEED;
			
			sendCharacterMovement(character);
		}
		
		character->zRot = M_PI;
	}
	else if (direction == LEFT)
	{
		if (characterIsOutOfBounds(direction, character) && checkCharacterCollision(direction, character, characterB, characterC, characterD))
		{
			character->x -= CHARACTER_SPEED;
			
			sendCharacterMovement(character);
		}
		
		character->zRot = 0.0;
	}
	else if (direction == DOWN)
	{
		if (characterIsOutOfBounds(direction, character) && checkCharacterCollision(direction, character, characterB, characterC, characterD))
		{
			character->y -= CHARACTER_SPEED;
			
			sendCharacterMovement(character);
		}
		
		character->zRot = 100.0 * (M_PI / 180.0f);
	}
	else if (direction == UP)
	{
		if (characterIsOutOfBounds(direction, character) && checkCharacterCollision(direction, character, characterB, characterC, characterD))
		{
			character->y += CHARACTER_SPEED;
			
			sendCharacterMovement(character);
		}
		
		character->zRot = 3.0f * M_PI / 2.0f;
	}
}

void turnCharacter(Character *character, int direction)
{
	if (direction == RIGHT)
	{
		character->zRot = M_PI;
	}
	else if (direction == LEFT)
	{
		character->zRot = 0.0;
	}
	else if (direction == DOWN)
	{
		character->zRot = 100.0 * (M_PI / 180.0f);
	}
	else if (direction == UP)
	{
		character->zRot = 3.0f * M_PI / 2.0f;
	}
	
	character->direction = direction;
}

void shootCharacterWeaponWithoutChecks(Character *character)
{
    bindWeaponWithCharacter(character);
	character->weap->drawingState = SDL_TRUE;
	character->weap->animationState = SDL_TRUE;
}

void shootCharacterWeapon(Character *character)
{
	if (!character->weap->animationState && gTiles[getTileIndexLocation((int)character->x, (int)character->y)].z == -25.0)
	{
		if (gNetworkConnection)
		{
			char shootWeaponMessage[256];
			char movementMessage[256];
			
			sprintf(shootWeaponMessage, "sw%i", IDOfCharacter(character));
			sprintf(movementMessage, "mo%i %f %f %f %i", IDOfCharacter(character), character->x, character->y, character->z, character->direction);
			
			if (gNetworkConnection->type == NETWORK_SERVER_TYPE)
			{
				sendToClients(0, movementMessage);
				sendToClients(0, shootWeaponMessage);
			}
			else if (gNetworkConnection->type == NETWORK_CLIENT_TYPE && gNetworkConnection->input->character == character)
			{
				sendto(gNetworkConnection->socket, shootWeaponMessage, strlen(shootWeaponMessage), 0, (struct sockaddr *)&gNetworkConnection->hostAddress, sizeof(gNetworkConnection->hostAddress));
			}
		}
		
		if (!gNetworkConnection || gNetworkConnection->type != NETWORK_CLIENT_TYPE)
		{
            shootCharacterWeaponWithoutChecks(character);
		}
	}
}

void bindWeaponWithCharacter(Character *character)
{
	// don't bind z coordinate value
	character->weap->x = character->x;
	character->weap->y = character->y;
	character->weap->direction = character->direction;
	
	// make sure character can't move while firing the weapon
	character->backup_direction = character->direction;
	character->direction = NO_DIRECTION;
}
