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

#include "collision.h"
#include "scenery.h"
#include "weapon.h"

/*
 * Checks to see if character a intersects with character b given the direction character a is moving towards.
 * Returns false if we can't move, otherwise returns true if we can move
 */
bool checkCharacterColl(Character *a, Character *b, int direction)
{
	if (!a->lives || !b->lives)
		return true;
	
	// we don't check collision if they're in different dimensions
	if (a->z != b->z)
		return true;
	
	if (direction == UP && a->y < b->y &&
		(a->x < b->x + 0.8f && a->x > b->x - 0.8f) &&
		a->y > b->y - 1.0f)
		return false;
	
	if (direction == DOWN && a->y > b->y &&
		(a->x < b->x + 0.8f && a->x > b->x - 0.8f) &&
		a->y < b->y + 1.0f)
		return false;
	
	if (direction == RIGHT && a->x < b->x &&
		(a->y < b->y + 0.8f && a->y > b->y - 0.8f) &&
		a->x > b->x - 1.1f)
		return false;
	
	if (direction == LEFT && a->x > b->x &&
		(a->y < b->y + 0.8f && a->y > b->y - 0.8f) &&
		a->x < b->x + 1.1f)
		return false;
	
	return true;
}

/* 
 * Checks if characterA with a given direction is colliding with characterB, characterC, or characterD.
 * Returns true if the characterA can move without colliding into character B, C, or D; otherwise, it returns false
 */
bool checkCharacterCollision(int direction, Character *characterA, Character *characterB, Character *characterC, Character *characterD)
{	
	if (characterA->direction == NO_DIRECTION || !checkCharacterColl(characterA, characterB, direction) || !checkCharacterColl(characterA, characterC, direction) || !checkCharacterColl(characterA, characterD, direction))
		return false;
	
	return true;
}

/*
 * Checks if the character will be out of bounds of the checkerboard if it moves within the given direction.
 * Returns false if character can't move, otherwise returns true
 */
bool characterCanMove(int direction, Character *character)
{	
	int x;
	int y;
	
	int index = 0;
	
	// trancuate values to integers.
	x = (int)character->x;
	y = (int)character->y;
	
	index = getTileIndexLocation(x, y);
	
	if (index < 0 || index >= NUMBER_OF_TILES)
	{
		return false;
	}
	
	int nextTileIndex;
	
	if (direction == LEFT && ((nextTileIndex = leftTileIndex(index)) == -1 || !gTiles[nextTileIndex].state || gTiles[nextTileIndex].isDead || gTiles[nextTileIndex].coloredID == IDOfCharacter(character)))
	{
		/*
		 * We know that the current tile's left tile is destroyed, but we only have access to the current tile.
		 * So we check if the character is < than the current tile's x coordinate - 0.7.
		 * To translate it to a valid value on the checkerboard, in this case, we add the current tile location
		 * by 7.0
		 */
		
		if (character->x < (gTiles[index].x + 7.0f) - 0.7f)
			return false;
	}
	
	else if (direction == RIGHT && ((nextTileIndex = rightTileIndex(index)) == -1 || !gTiles[nextTileIndex].state || gTiles[nextTileIndex].isDead || gTiles[nextTileIndex].coloredID == IDOfCharacter(character)))
	{
		
		if (character->x > (gTiles[index].x + 7.0f) + 0.7f)
			return false;
	}
	
	else if (direction == DOWN && ((nextTileIndex = downTileIndex(index)) == -1 || !gTiles[nextTileIndex].state || gTiles[nextTileIndex].isDead || gTiles[nextTileIndex].coloredID == IDOfCharacter(character)))
	{
		
		if (character->y < ((gTiles[index].y - 18.5f) + 6.0f) - 0.7f)
			return false;
	}
	
	else if (direction == UP && ((nextTileIndex = upTileIndex(index)) == -1 || !gTiles[nextTileIndex].state || gTiles[nextTileIndex].isDead || gTiles[nextTileIndex].coloredID == IDOfCharacter(character)))
	{
		
		if (character->y > ((gTiles[index].y - 18.5f) + 6.0f) + 0.7f)
			return false;
	}
	
	return true;
}

/* x, y arguements are the specified character's x and y values */
int getTileIndexLocation(int x, int y)
{
	// Round x and y values to the tile they're on.
	// Initially, each tile contains two numbers of values.
	// Example: tile 2 can have a location of 1.x or 2.x
	// For each x and y component..
	// 0 -> 0
	// 1 -> 1
	// 2 -> 1
	// 3 -> 2
	// 4 -> 2
	// 5 -> 3
	// etc..
	
	int tileX = (x + 1) / 2;
	int tileY = (y + 1) / 2;
	
	return tileX + (tileY * 8);
}

// Returns the row index the character is on
int rowOfCharacter(Character *character)
{
	int tileIndex = getTileIndexLocation((int)character->x, (int)character->y);
	return tileIndex / 8;
}

// Returns the column index the character is on
int columnOfCharacter(Character *character)
{
	int tileIndex = getTileIndexLocation((int)character->x, (int)character->y);
	return tileIndex % 8;
}
