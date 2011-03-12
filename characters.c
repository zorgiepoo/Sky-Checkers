/*
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
 
 * Coder: Mayur Pawashe (Zorg)
 * http://zorg.tejat.net
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

static const float CHARACTER_SPEED =	0.08;

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

// Display for the charcter's icons. Used when displaying how many lives the characters have
static GLuint gIconList;

// Icon quadric used for the character's icons that we texture map
static GLUquadricObj *gIconQuadric;

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
	gRedRover.weap->red = 1.0;
	gRedRover.weap->green = 0.0;
	gRedRover.weap->blue = 0.0;
	gRedRover.netState = NETWORK_NO_STATE;
	gRedRover.netName = NULL;
	gRedRover.backup_state = 0;
	
	// greenTree
	loadCharacter(&gGreenTree);
	gGreenTree.weap = malloc(sizeof(Weapon));
	initWeapon(gGreenTree.weap);
	gGreenTree.weap->red = 0.2196;
	gGreenTree.weap->green = 0.851;
	gGreenTree.weap->blue = 0.2623;
	gGreenTree.netState = NETWORK_NO_STATE;
	gGreenTree.netName = NULL;
	gGreenTree.backup_state = 0;
	
	// pinkBubbleGum
	loadCharacter(&gPinkBubbleGum);
	gPinkBubbleGum.weap = malloc(sizeof(Weapon));
	initWeapon(gPinkBubbleGum.weap);
	gPinkBubbleGum.weap->red = 0.988;
	gPinkBubbleGum.weap->green = 0.486;
	gPinkBubbleGum.weap->blue = 0.796;
	gPinkBubbleGum.netState = NETWORK_NO_STATE;
	gPinkBubbleGum.netName = NULL;
	gPinkBubbleGum.backup_state = 0;
	
	// blueLightning
	loadCharacter(&gBlueLightning);
	gBlueLightning.weap = malloc(sizeof(Weapon));
	initWeapon(gBlueLightning.weap);
	gBlueLightning.weap->red = 0.09;
	gBlueLightning.weap->green = 0.4157;
	gBlueLightning.weap->blue = 0.5019;
	gBlueLightning.netState = NETWORK_NO_STATE;
	gBlueLightning.netName = NULL;
	gBlueLightning.backup_state = 0;
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

void loadCharacterTextures(void)
{
	loadTexture("Data/Textures/face.bmp", &gCharacterTex);
	
	// Create Quadrics.
	gRedRover.quadric = gluNewQuadric();
	gGreenTree.quadric = gluNewQuadric();
	gPinkBubbleGum.quadric = gluNewQuadric();
	gBlueLightning.quadric = gluNewQuadric();
	
	gIconQuadric = gluNewQuadric();
}

void deleteCharacterTextures(void)
{
	glDeleteTextures(1, &gCharacterTex);
}

void buildCharacterLists(void)
{
	// RedRover
	gRedRover.characterList = glGenLists(1);
	glNewList(gRedRover.characterList, GL_COMPILE);
	glBindTexture(GL_TEXTURE_2D, gCharacterTex);
	gluQuadricTexture(gRedRover.quadric, GL_TRUE);
	
	glColor3f(0.9, 0.0, 0.0);
	gluSphere(gRedRover.quadric, 0.6, 30.0, 30.0);
	
	glEndList();
	
	// GreenTree
	gGreenTree.characterList = glGenLists(1);
	glNewList(gGreenTree.characterList, GL_COMPILE);
	glBindTexture(GL_TEXTURE_2D, gCharacterTex);
	gluQuadricTexture(gGreenTree.quadric, GL_TRUE);
	
	glColor3f(0.3, 1.0, 0.3);
	gluSphere(gGreenTree.quadric, 0.6, 30.0, 30.0);
	
	glEndList();
	
	// PinkBubbleGum
	gPinkBubbleGum.characterList = glGenLists(1);
	glNewList(gPinkBubbleGum.characterList, GL_COMPILE);
	glBindTexture(GL_TEXTURE_2D, gCharacterTex);
	gluQuadricTexture(gPinkBubbleGum.quadric, GL_TRUE);
	
	glColor3f(1.0, 0.6, 0.6);
	gluSphere(gPinkBubbleGum.quadric, 0.6, 30.0, 30.0);
	
	glEndList();
	
	// BlueLightning
	gBlueLightning.characterList = glGenLists(1);
	glNewList(gBlueLightning.characterList, GL_COMPILE);
	glBindTexture(GL_TEXTURE_2D, gCharacterTex);
	gluQuadricTexture(gBlueLightning.quadric, GL_TRUE);
	
	glColor3f(0.3, 0.5, 0.6);
	gluSphere(gBlueLightning.quadric, 0.6, 30.0, 30.0);
	
	glEndList();
	
	// Icon
	gIconList = glGenLists(1);
	glNewList(gIconList, GL_COMPILE);
	glBindTexture(GL_TEXTURE_2D, gCharacterTex);
	gluQuadricTexture(gIconQuadric, GL_TRUE);
	
	gluDisk(gIconQuadric, 0.0, 0.4, 30, 5);
	
	glEndList();
	
	// Delete the quadrics. They're not needed anymore.
	gluDeleteQuadric(gRedRover.quadric);
	gluDeleteQuadric(gGreenTree.quadric);
	gluDeleteQuadric(gPinkBubbleGum.quadric);
	gluDeleteQuadric(gBlueLightning.quadric);
	
	gluDeleteQuadric(gIconQuadric);
}

void deleteCharacterLists(void)
{
	glDeleteLists(gRedRover.characterList, 1);
	glDeleteLists(gGreenTree.characterList, 1);
	glDeleteLists(gPinkBubbleGum.characterList, 1);
	glDeleteLists(gBlueLightning.characterList, 1);
	
	glDeleteLists(gIconList, 1);
}

void drawCharacter(Character *character)
{
	// don't draw the character if he's not on in the scene
	if (character->z <= -170.0)
		return;
	
	glEnable(GL_TEXTURE_2D);
	
	glRotatef(-40, 1.0, 0.0, 0.0);
	glTranslatef(-7.0, 12.5, -25.0);
	
	glTranslatef(character->x, character->y, character->z);
	
	glRotatef(character->xRot, 1.0, 0.0, 0.0);
	glRotatef(character->yRot, 0.0, 1.0, 0.0);
	// This rotatation is to control direction.
	glRotatef(character->zRot, 0.0, 0.0, 1.0);
	
	glCallList(character->characterList);
	
	glDisable(GL_TEXTURE_2D);
	
	glLoadIdentity();
}

void drawRedRoverIcon(void)
{
	glColor4f(0.9, 0.0, 0.0, 0.7);
	
	glRotatef(180.0, 1.0, 0.0, 0.0);
	glCallList(gIconList);
	glRotatef(180.0, 1.0, 0.0, 0.0);
}

void drawPinkBubbleGumIcon(void)
{
	glColor4f(1.0, 0.6, 0.6, 0.7);
	
	glRotatef(180.0, 1.0, 0.0, 0.0);
	glCallList(gIconList);
	glRotatef(180.0, 1.0, 0.0, 0.0);
}

void drawGreenTreeIcon(void)
{
	glColor4f(0.3, 1.0, 0.3, 0.7);
	
	glRotatef(180.0, 1.0, 0.0, 0.0);
	glCallList(gIconList);
	glRotatef(180.0, 1.0, 0.0, 0.0);
}

void drawBlueLightningIcon(void)
{
	glColor4f(0.1, 0.5, 1.0, 0.7);
	
	glRotatef(180.0, 1.0, 0.0, 0.0);
	glCallList(gIconList);
	glRotatef(180.0, 1.0, 0.0, 0.0);
}

void drawCharacterIcons(void)
{
	// pinkBubbleGum icon
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glTranslatef(-9.0, -9.5, -25.0);
	
	drawPinkBubbleGumIcon();
	
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	
	glLoadIdentity();
	
	// redRover icon
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glTranslatef(-3.67, -9.5, -25.0);
	
	drawRedRoverIcon();
	
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	
	glLoadIdentity();
	
	// greenTree icon
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glTranslatef(1.63, -9.5, -25.0);
	
	drawGreenTreeIcon();
	
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	
	glLoadIdentity();
	
	// BlueLightning icon
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glTranslatef(6.93, -9.5, -25.0);
	
	drawBlueLightningIcon();
	
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	
	glLoadIdentity();
}

static void drawCharacterLive(Character *character, char *playerNumberString)
{
	if (character->lives != 0)
	{
		drawStringf(0.5, 0.5, "%i", character->lives);
	}
	
	glTranslatef(2.0, 0.0, 0.0);
	
	if (gNetworkConnection)
	{
		if (character->netName)
		{
			drawString(1.0, 0.5, character->netName);
		}
		// this is a clever way to see if this character is going to be an AI or a networked player
		else if ((gNetworkConnection->type == NETWORK_SERVER_TYPE && character->netState == NETWORK_PLAYING_STATE) ||
				 (gNetworkConnection->type == NETWORK_CLIENT_TYPE && gPinkBubbleGum.netState == NETWORK_PLAYING_STATE))
		{
			drawString(1.0, 0.5, "[AI]");
		}
	}
	else
	{
		if (character->state == CHARACTER_AI_STATE)
		{
			drawString(1.0, 0.5, "[AI]");
		}
		else
		{
			drawString(1.0, 0.5, playerNumberString);
		}
	}
	
	glLoadIdentity();
}

void drawCharacterLives(void)
{	
	// PinkBubbleGum
	glTranslatef(-12.3, -14.4, -38.0);
	glColor4f(1.0, 0.6, 0.6, 1.0);
	
	drawCharacterLive(&gPinkBubbleGum, "[P1]");
	
	// RedRover
	glTranslatef(-4.2, -14.4, -38.0);
	glColor4f(0.9, 0.0, 0.0, 1.0);
	
	drawCharacterLive(&gRedRover, "[P2]");
	
	// GreenTree
	glTranslatef(3.9, -14.4, -38.0);
	glColor4f(0.0, 1.0, 0.0, 1.0);
	
	drawCharacterLive(&gGreenTree, "[P3]");
	
	// BlueLightning
	glTranslatef(12.0, -14.4, -38.0);
	glColor4f(0.0, 0.0, 1.0, 1.0);
	
	drawCharacterLive(&gBlueLightning, "[P4]");
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
		
		isFree = availableTile(randOne, randTwo);
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
		character->zRot = 90.0;
		character->xRot = 170.0;
		character->yRot = 0.0;
	}
	else if (character->direction == LEFT)
	{
		character->zRot = 270.0;
		character->xRot = 170.0;
		character->yRot = 0.0;
	}
	else if (character->direction == DOWN)
	{
		character->zRot = 180.0;
		character->xRot = 170.0;
		character->yRot = 0.0;
	}
	else if (character->direction == UP)
	{
		character->zRot = 180.0;
		character->yRot = 180.0;
		character->xRot = 355.0;
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
		
		character->zRot = 90.0;
		character->xRot = 170.0;
		character->yRot = 0.0;
	}
	else if (direction == LEFT)
	{
		if (characterIsOutOfBounds(direction, character) && checkCharacterCollision(direction, character, characterB, characterC, characterD))
		{
			character->x -= CHARACTER_SPEED;
			
			sendCharacterMovement(character);
		}
		
		character->zRot = 270.0;
		character->xRot = 170.0;
		character->yRot = 0.0;
	}
	else if (direction == DOWN)
	{
		if (characterIsOutOfBounds(direction, character) && checkCharacterCollision(direction, character, characterB, characterC, characterD))
		{
			character->y -= CHARACTER_SPEED;
			
			sendCharacterMovement(character);
		}
		
		character->zRot = 180.0;
		character->xRot = 170.0;
		character->yRot = 0.0;
	}
	else if (direction == UP)
	{
		if (characterIsOutOfBounds(direction, character) && checkCharacterCollision(direction, character, characterB, characterC, characterD))
		{
			character->y += CHARACTER_SPEED;
			
			sendCharacterMovement(character);
		}
		
		character->zRot = 180.0;
		character->yRot = 180.0;
		character->xRot = 355.0;
	}
}

void turnCharacter(Character *character, int direction)
{
	if (direction == RIGHT)
	{
		character->zRot = 90.0;
		character->xRot = 170.0;
		character->yRot = 0.0;
	}
	else if (direction == LEFT)
	{
		character->zRot = 270.0;
		character->xRot = 170.0;
		character->yRot = 0.0;
	}
	else if (direction == DOWN)
	{
		character->zRot = 180.0;
		character->xRot = 170.0;
		character->yRot = 0.0;
	}
	else if (direction == UP)
	{
		character->zRot = 180.0;
		character->yRot = 180.0;
		character->xRot = 355.0;
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
	if (!character->weap->animationState && gTiles[getTileIndexLocation(character->x, character->y)].z == -25.0)
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
