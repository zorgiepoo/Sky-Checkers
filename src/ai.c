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
#include "mt_random.h"
#include "scenery.h"
#include "time.h"
#include "globals.h"

#include <stdlib.h>

static void setNewDirection(Character *character);
static void directCharacterBasedOnCollisions(Character *character, float currentTime);

static void shootWeaponProjectile(Character *character, float currentTime);

static void fireAIWeapon(Character *character);

static void attackCharacterOnRow(Character *character, Character *characterB, float currentTime);
static void attackCharacterOnColumn(Character *character, Character *characterB, float currentTime);

static void avoidCharacter(Character *character, Character *characterB, float currentTime);

static void updateDirectionAndMoveTimer(Character *character, float currentTime)
{
	setNewDirection(character);
	character->move_timer = currentTime + (float)((mt_random() % 2) + 1);
}

void updateAI(Character *character, float currentTime, double timeDelta)
{
	if (!CHARACTER_IS_ALIVE(character) || character->state != CHARACTER_AI_STATE || !character->active || !character->lives || (gNetworkConnection && gNetworkConnection->type == NETWORK_CLIENT_TYPE))
		return;
	
	if (character->direction == NO_DIRECTION || currentTime > character->move_timer)
	{
		updateDirectionAndMoveTimer(character, currentTime);
	}
	
	directCharacterBasedOnCollisions(character, currentTime);
	
	if (!gNetworkConnection || (gRedRover.netState == NETWORK_PLAYING_STATE && gGreenTree.netState == NETWORK_PLAYING_STATE && gBlueLightning.netState == NETWORK_PLAYING_STATE))
	{
		character->fire_timer += (float)timeDelta;
		shootWeaponProjectile(character, currentTime);
	}
}

static void setNewDirection(Character *character)
{
	int column = columnOfCharacter(character);
	int row = rowOfCharacter(character);
	
	if (row >= 0 && row < 8 && column >= 0 && column < 8)
	{
		bool unableToMoveLeft = column == 0 || !gTiles[row * 8 + (column - 1)].state || gTiles[row * 8 + (column - 1)].isDead;
		bool unableToMoveRight = column == 7 || !gTiles[row * 8 + (column + 1)].state || gTiles[row * 8 + (column + 1)].isDead;
		bool unableToMoveDown = row == 0 || !gTiles[(row - 1) * 8 + column].state || gTiles[(row - 1) * 8 + column].isDead;
		bool unableToMoveUp = row == 7 || !gTiles[(row + 1) * 8 + column].state || gTiles[(row + 1) * 8 + column].isDead;
		
		if (unableToMoveLeft && unableToMoveRight && unableToMoveDown && unableToMoveUp)
		{
			character->direction = NO_DIRECTION;
		}
		else if (character->direction == UP || character->direction == DOWN)
		{
			// set new direction to either LEFT or RIGHT
			if (unableToMoveLeft)
			{
				character->direction = RIGHT;
			}
			else if (unableToMoveRight)
			{
				character->direction = LEFT;
			}
			else
			{
				character->direction = (mt_random() % 2) + 1;
			}
		}
		else /* if (*direction == RIGHT || *direction == LEFT || *direction == NO_DIRECTION) */
		{
			// set new direction to either UP or DOWN
			if (unableToMoveDown)
			{
				character->direction = UP;
			}
			else if (unableToMoveUp)
			{
				character->direction = DOWN;
			}
			else
			{
				character->direction = (mt_random() % 2) + 3;
			}
		}
	}
}

static void avoidCharacter(Character *character, Character *characterB, float currentTime)
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
		updateDirectionAndMoveTimer(character, currentTime);
	}
}

static void directCharacterBasedOnCollisions(Character *character, float currentTime)
{
	Character *characterB;
	Character *characterC;
	Character *characterD;
	
	getOtherCharacters(character, &characterB, &characterC, &characterD);
	
	if (!characterCanMove(character->direction, character))
	{
		updateDirectionAndMoveTimer(character, currentTime);
	}
	
	avoidCharacter(character, characterB, currentTime);
	avoidCharacter(character, characterC, currentTime);
	avoidCharacter(character, characterD, currentTime);
	
	/* The AI should avoid dieing from the dieing stones */
	
	int tileLocation = getTileIndexLocation((int)character->x, (int)character->y);
	if (tileLocation >= 0 && tileLocation < NUMBER_OF_TILES)
	{
		if (gTiles[tileLocation].coloredID == DIEING_STONE_ID)
		{
			int row = rowOfCharacter(character);
			int column = columnOfCharacter(character);
			
			if (row == 0 || row == 1)
			{
				int upTileLocation = upTileIndex(tileLocation);
				if (upTileLocation != -1 && !gTiles[upTileLocation].isDead && gTiles[upTileLocation].state)
					character->direction = UP;
			}
			else if (row == 7 || row == 6)
			{
				int downTileLocation = downTileIndex(tileLocation);
				if (downTileLocation != -1 && !gTiles[downTileLocation].isDead && gTiles[downTileLocation].state)
					character->direction = DOWN;
			}
			else if (column == 0 || column == 1)
			{
				int rightTileLocation = rightTileIndex(tileLocation);
				if (rightTileLocation != -1 && !gTiles[rightTileLocation].isDead && gTiles[rightTileLocation].state)
					character->direction = RIGHT;
			}
			else if (column == 7 || column == 6)
			{
				int leftTileLocation = leftTileIndex(tileLocation);
				if (leftTileLocation != -1 && !gTiles[leftTileLocation].isDead && gTiles[leftTileLocation].state)
					character->direction = LEFT;
			}
		}
	}
}

static void shootWeaponProjectile(Character *character, float currentTime)
{
	int AIMode = gAIMode;
	
	float timeAliveThreshold;
	if (AIMode == AI_HARD_MODE)
	{
		timeAliveThreshold = 1.0f;
	}
	else if (AIMode == AI_MEDIUM_MODE)
	{
		timeAliveThreshold = 2.0f;
	}
	else
	{
		timeAliveThreshold = 3.0f;
	}
	
	if (character->time_alive < timeAliveThreshold || character->fire_timer < 0.25f || character->weap->animationState || !gGameHasStarted)
		return;
	
	character->fire_timer = 0.0f;
	
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
	
	// AI should be worrying about escaping from the falling dieing stones
	if (gTiles[tileIndex].coloredID == DIEING_STONE_ID)
	{
		return;
	}
	
	// If the tile the AI is on has been colored for a while, don't let them react fast enough to fire back
	uint32_t ticks = ZGGetTicks();
	if (gTiles[tileIndex].colorTime > 0 && ticks >= gTiles[tileIndex].colorTime + 0.35f)
	{
		return;
	}
	
	unsigned int exitOutValue;
	if (AIMode == AI_HARD_MODE)
	{
		exitOutValue = 1;
	}
	else if (AIMode == AI_MEDIUM_MODE)
	{
		exitOutValue = 5;
	}
	else
	{
		exitOutValue = 8;
	}
	
	// Exit out of function randomly depending on AI difficulty.
	// An AIMode of AI_EASY_MODE will exit out more than AI_HARD_MODE
	if ((mt_random() % 10) + 1 <= exitOutValue)
	{
		return;
	}
	
	if (characterB->time_alive >= timeAliveThreshold && rowOfCharacter(character) == rowOfCharacter(characterB))
	{
		attackCharacterOnRow(character, characterB, currentTime);
	}
	else if (characterC->time_alive >= timeAliveThreshold && rowOfCharacter(character) == rowOfCharacter(characterC))
	{
		attackCharacterOnRow(character, characterC, currentTime);
	}
	else if (characterD->time_alive >= timeAliveThreshold && rowOfCharacter(character) == rowOfCharacter(characterD))
	{
		attackCharacterOnRow(character, characterD, currentTime);
	}
	else if (characterB->time_alive >= timeAliveThreshold && columnOfCharacter(character) == columnOfCharacter(characterB))
	{
		attackCharacterOnColumn(character, characterB, currentTime);
	}
	else if (characterC->time_alive >= timeAliveThreshold && columnOfCharacter(character) == columnOfCharacter(characterC))
	{
		attackCharacterOnColumn(character, characterC, currentTime);
	}
	else if (characterD->time_alive >= timeAliveThreshold && columnOfCharacter(character) == columnOfCharacter(characterD))
	{
		attackCharacterOnColumn(character, characterD, currentTime);
	}
	else if ((mt_random() % 2) == 0)
	{
		if (characterB->time_alive >= timeAliveThreshold && abs(rowOfCharacter(character) - rowOfCharacter(characterB)) <= 1)
		{
			attackCharacterOnRow(character, characterB, currentTime);
		}
		else if (characterC->time_alive >= timeAliveThreshold && abs(rowOfCharacter(character) - rowOfCharacter(characterC)) <= 1)
		{
			attackCharacterOnRow(character, characterC, currentTime);
		}
		else if (characterD->time_alive >= timeAliveThreshold && abs(rowOfCharacter(character) - rowOfCharacter(characterD)) <= 1)
		{
			attackCharacterOnRow(character, characterD, currentTime);
		}
		else if (characterB->time_alive >= timeAliveThreshold && abs(columnOfCharacter(character) - columnOfCharacter(characterB)) <= 1)
		{
			attackCharacterOnColumn(character, characterB, currentTime);
		}
		else if (characterC->time_alive >= timeAliveThreshold && abs(columnOfCharacter(character) - columnOfCharacter(characterC)) <= 1)
		{
			attackCharacterOnColumn(character, characterC, currentTime);
		}
		else if (characterD->time_alive >= timeAliveThreshold && abs(columnOfCharacter(character) - columnOfCharacter(characterD)) <= 1)
		{
			attackCharacterOnColumn(character, characterD, currentTime);
		}
	}
}

static void fireAIWeapon(Character *character)
{
	prepareFiringCharacterWeapon(character, character->x, character->y, character->pointing_direction, 0.0f);
}

static void attackCharacterOnRow(Character *character, Character *characterB, float currentTime)
{
	if (character->x > characterB->x)
	{
		turnCharacter(character, LEFT);
		character->direction = LEFT;
		fireAIWeapon(character);
		
		updateDirectionAndMoveTimer(character, currentTime);
	}
	else if (character->x < characterB->x)
	{
		turnCharacter(character, RIGHT);
		character->direction = RIGHT;
		fireAIWeapon(character);
		
		updateDirectionAndMoveTimer(character, currentTime);
	}
}

static void attackCharacterOnColumn(Character *character, Character *characterB, float currentTime)
{
	if (character->y > characterB->y)
	{
		turnCharacter(character, DOWN);
		character->direction = DOWN;
		fireAIWeapon(character);
		
		updateDirectionAndMoveTimer(character, currentTime);
	}
	else if (character->y < characterB->y)
	{
		turnCharacter(character, UP);
		character->direction = UP;
		fireAIWeapon(character);
		
		updateDirectionAndMoveTimer(character, currentTime);
	}
}
