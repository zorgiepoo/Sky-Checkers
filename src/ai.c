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

void moveAI(Character *character, int currentTime)
{
	if (character->z != 2.0 || character->state != CHARACTER_AI_STATE || character->direction == NO_DIRECTION || !character->lives || (gNetworkConnection && gNetworkConnection->type == NETWORK_CLIENT_TYPE))
		return;
	
	if (currentTime > character->ai_timer)
	{
		setNewDirection(&character->direction);
		character->ai_timer = currentTime + (mt_random() % 2) + 1;
	}
	
	moveCharacter(character, character->direction);
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
	
	Tile *tile = &gTiles[getTileIndexLocation((int)character->x, (int)character->y)];
	// gray color red == 0.31
	if (fabs(tile->red - 0.31) < 0.00001)
	{
		int row = rowOfCharacter(character);
		int column = columnOfCharacter(character);
		
		if (row == 1 || row == 2)
		{
			if (tile->up && !tile->up->isDead && tile->up->state)
				character->direction = UP;
		}
		else if (row == 8 || row == 7)
		{
			if (tile->down && !tile->down->isDead && tile->down->state)
				character->direction = DOWN;
		}
		else if (column == 1 || column == 2)
		{
			if (tile->right && !tile->right->isDead && tile->right->state)
				character->direction = RIGHT;
		}
		else if (column == 8 || column == 7)
		{
			if (tile->left && !tile->left->isDead && tile->left->state)
				character->direction = LEFT;
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

// Returns the row the character is on, otherwise, -1 on error
static int rowOfCharacter(Character *character)
{	
	int tileIndex;
	int row = -1;
	
	tileIndex = getTileIndexLocation((int)character->x, (int)character->y);
	
	if (tileIndex < 9)
		row = 1;
	else if (tileIndex < 17)
		row = 2;
	else if (tileIndex < 25)
		row = 3;
	else if (tileIndex < 33)
		row = 4;
	else if (tileIndex < 41)
		row = 5;
	else if (tileIndex < 49)
		row = 6;
	else if (tileIndex < 57)
		row = 7;
	else
		row = 8;
	
	return row;
}

// Returns the column number of which the character is on, otherwise, -1 on error
static int columnOfCharacter(Character *character)
{
	int tileIndex;
	int column;
	
	tileIndex = getTileIndexLocation((int)character->x, (int)character->y);
	
	for (column = 1; column <= 8; column++)
	{
		int rowIndex = column;
		
		while (rowIndex <= 64)
		{
			rowIndex += 8;
			
			if (rowIndex == tileIndex)
			{
				return column;
			}
		}
	}
	
	return -1;
}

static void fireAIWeapon(Character *character)
{
	shootCharacterWeapon(character);
}

static void attackCharacterOnRow(Character *character, Character *characterB, int currentTime)
{
	if (character->x > characterB->x && character->direction != LEFT)
	{
		moveCharacter(character, LEFT);
		fireAIWeapon(character);
		
		// the character's direction before firing is stored in backup_direction, this will revert itself afterwards
		setNewDirection(&character->backup_direction);
		character->ai_timer = currentTime + (mt_random() % 2) + 1;
	}
	else if (character->x < characterB->x && character->direction != RIGHT)
	{
		moveCharacter(character, RIGHT);
		fireAIWeapon(character);
		
		// the character's direction before firing is stored in backup_direction, this will revert itself afterwards
		setNewDirection(&character->backup_direction);
		character->ai_timer = currentTime + (mt_random() % 2) + 1;
	}
}

static void attackCharacterOnColumn(Character *character, Character *characterB, int currentTime)
{
	if (character->y > characterB->y && character->direction != DOWN)
	{
		moveCharacter(character, DOWN);
		fireAIWeapon(character);
		
		// the character's direction before firing is stored in backup_direction, this will revert itself afterwards
		setNewDirection(&character->backup_direction);
		character->ai_timer = currentTime + (mt_random() % 2) + 1;
	}
	else if (character->y < characterB->y && character->direction != UP)
	{
		moveCharacter(character, UP);
		fireAIWeapon(character);
		
		// the character's direction before firing is stored in backup_direction, this will revert itself afterwards
		setNewDirection(&character->backup_direction);
		character->ai_timer = currentTime + (mt_random() % 2) + 1;
	}
}
