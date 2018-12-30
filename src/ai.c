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

#include "ai.h"
#include "characters.h"
#include "collision.h"
#include "network.h"
#include "utilities.h"

static void setNewDirection(int *direction);
static void directCharacterBasedOnCollisions(Character *character, int currentTime);

static void shootWeaponProjectile(Character *character, int currentTime);

static int rowOfCharacter(Character *character);
static int columnOfCharacter(Character *character);

static void fireAIWeapon(Character *character);

static void attackCharacterOnRow(Character *character, Character *characterB, int currentTime);
static void attackCharacterOnColumn(Character *character, Character *characterB, int currentTime);

static void avoidCharacter(Character *character, Character *characterB, int currentTime);

void updateAI(Character *character, int currentTime, double timeDelta)
{
	if (!CHARACTER_IS_ALIVE(character) || character->state != CHARACTER_AI_STATE || !character->active || !character->lives || (gNetworkConnection && gNetworkConnection->type == NETWORK_CLIENT_TYPE))
		return;
	
	if (character->direction == NO_DIRECTION || currentTime > character->ai_timer)
	{
		setNewDirection(&character->direction);
		character->ai_timer = currentTime + (mt_random() % 2) + 1;
	}
	
	directCharacterBasedOnCollisions(character, currentTime);
	
	if (!gNetworkConnection || (gRedRover.netState == NETWORK_PLAYING_STATE && gGreenTree.netState == NETWORK_PLAYING_STATE && gBlueLightning.netState == NETWORK_PLAYING_STATE))
	{
		shootWeaponProjectile(character, currentTime);
	}
}

static void setNewDirection(int *direction)
{
	if (*direction == UP || *direction == DOWN)
	{
		// set it to either LEFT or RIGHT
		*direction = (mt_random() % 2) + 1;
	}
	else /* if (*direction == RIGHT || *direction == LEFT) */
	{
		// set it to either UP or DOWN
		*direction = (mt_random() % 2) + 3;
	}
}

static void avoidCharacter(Character *character, Character *characterB, int currentTime)
{
	if (!checkCharacterColl(character, characterB, character->direction)									&&
		(characterB->state != CHARACTER_AI_STATE															||
		(
		 (character->direction == DOWN && characterB->direction == UP)										||
		 (character->direction == RIGHT && characterB->direction == LEFT)									||
		 (character->direction == UP && characterB->direction == UP) /* character is on bottom side */		||
		 (character->direction == DOWN && characterB->direction == DOWN) /* character is on top sode */		||
		 (character->direction == RIGHT && characterB->direction == RIGHT) /*character is on left side */	||
		 (character->direction == LEFT && characterB->direction == LEFT) /* character is on right side */
		)))
	{
		setNewDirection(&character->direction);
		character->ai_timer = currentTime + (mt_random() % 2) + 1;
	}
}

static void directCharacterBasedOnCollisions(Character *character, int currentTime)
{
	Character *characterB;
	Character *characterC;
	Character *characterD;
	
	getOtherCharacters(character, &characterB, &characterC, &characterD);
	
	if (!characterIsOutOfBounds(character->direction, character))
	{
		setNewDirection(&character->direction);
		character->ai_timer = currentTime + (mt_random() % 2) + 1;
	}
	
	avoidCharacter(character, characterB, currentTime);
	avoidCharacter(character, characterC, currentTime);
	avoidCharacter(character, characterD, currentTime);
	
	/* The AI should avoid dieing from the gray stones */
	
	int tileLocation = getTileIndexLocation((int)character->x, (int)character->y);
	if (tileLocation >= 0 && tileLocation < NUMBER_OF_TILES)
	{
		Tile *tile = &gTiles[tileLocation];
		// gray color red == 0.31
		if (fabs(tile->red - 0.31) < 0.00001)
		{
			int row = rowOfCharacter(character);
			int column = columnOfCharacter(character);
			
			if (row == 0 || row == 1)
			{
				if (tile->up && !tile->up->isDead && tile->up->state)
					character->direction = UP;
			}
			else if (row == 7 || row == 6)
			{
				if (tile->down && !tile->down->isDead && tile->down->state)
					character->direction = DOWN;
			}
			else if (column == 0 || column == 1)
			{
				if (tile->right && !tile->right->isDead && tile->right->state)
					character->direction = RIGHT;
			}
			else if (column == 7 || column == 6)
			{
				if (tile->left && !tile->left->isDead && tile->left->state)
					character->direction = LEFT;
			}
		}
	}
}

static void shootWeaponProjectile(Character *character, int currentTime)
{
	int AIMode = gNetworkConnection ? gAINetMode : gAIMode;
	
	if (character->time_alive < AIMode || character->weap->animationState || !gGameHasStarted)
		return;
	
	Character *characterB;
	Character *characterC;
	Character *characterD;
	
	getOtherCharacters(character, &characterB, &characterC, &characterD);
	
	int tileIndex = getTileIndexLocation((int)character->x, (int)character->y);
	
	if (tileIndex < 0 || tileIndex >= NUMBER_OF_TILES)
	{
		return;
	}
	
	// AI should be worrying about not colliding into another character
	if (tileIndex == getTileIndexLocation((int)characterB->x, (int)characterB->y) ||
		tileIndex == getTileIndexLocation((int)characterC->x, (int)characterC->y) ||
		tileIndex == getTileIndexLocation((int)characterD->x, (int)characterD->y))
	{
		return;
	}
	
	// AI should be worrying about escaping from the falling gray stones
	if (fabs(gTiles[tileIndex].red - 0.31) < 0.00001)
	{
		return;
	}
	
	// Exit out of function randomly depending on AI difficulty.
	// Obviously, an AIMode of AI_EASY_MODE will exit out more than AI_HARD_MODE
	if ((mt_random() % 10) + 1 <= (unsigned)AIMode)
	{
		return;
	}
	
	if (characterB->time_alive >= AIMode && rowOfCharacter(character) == rowOfCharacter(characterB))
	{
		attackCharacterOnRow(character, characterB, currentTime);
	}
	else if (characterC->time_alive >= AIMode && rowOfCharacter(character) == rowOfCharacter(characterC))
	{
		attackCharacterOnRow(character, characterC, currentTime);
	}
	else if (characterD->time_alive >= AIMode && rowOfCharacter(character) == rowOfCharacter(characterD))
	{
		attackCharacterOnRow(character, characterD, currentTime);
	}
	else if (characterB->time_alive >= AIMode && columnOfCharacter(character) == columnOfCharacter(characterB))
	{
		attackCharacterOnColumn(character, characterB, currentTime);
	}
	else if (characterC->time_alive >= AIMode && columnOfCharacter(character) == columnOfCharacter(characterC))
	{
		attackCharacterOnColumn(character, characterC, currentTime);
	}
	else if (characterD->time_alive >= AIMode && columnOfCharacter(character) == columnOfCharacter(characterD))
	{
		attackCharacterOnColumn(character, characterD, currentTime);
	}
}

// Returns the row index the character is on
static int rowOfCharacter(Character *character)
{
	int tileIndex = getTileIndexLocation((int)character->x, (int)character->y);
	return tileIndex / 8;
}

// Returns the column index the character is on
static int columnOfCharacter(Character *character)
{
	int tileIndex = getTileIndexLocation((int)character->x, (int)character->y);
	return tileIndex % 8;
}

static void fireAIWeapon(Character *character)
{
	prepareFiringCharacterWeapon(character, character->x, character->y);
}

static void attackCharacterOnRow(Character *character, Character *characterB, int currentTime)
{
	if (character->x > characterB->x && character->direction != LEFT)
	{
		turnCharacter(character, LEFT);
		character->direction = LEFT;
		fireAIWeapon(character);
		
		setNewDirection(&character->direction);
		character->ai_timer = currentTime + (mt_random() % 2) + 1;
	}
	else if (character->x < characterB->x && character->direction != RIGHT)
	{
		turnCharacter(character, RIGHT);
		character->direction = RIGHT;
		fireAIWeapon(character);
		
		setNewDirection(&character->direction);
		character->ai_timer = currentTime + (mt_random() % 2) + 1;
	}
}

static void attackCharacterOnColumn(Character *character, Character *characterB, int currentTime)
{
	if (character->y > characterB->y && character->direction != DOWN)
	{
		turnCharacter(character, DOWN);
		character->direction = DOWN;
		fireAIWeapon(character);
		
		setNewDirection(&character->direction);
		character->ai_timer = currentTime + (mt_random() % 2) + 1;
	}
	else if (character->y < characterB->y && character->direction != UP)
	{
		turnCharacter(character, UP);
		character->direction = UP;
		fireAIWeapon(character);
		
		setNewDirection(&character->direction);
		character->ai_timer = currentTime + (mt_random() % 2) + 1;
	}
}
