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
 * Returns SDL_FALSE if we can't move, otherwise returns SDL_TRUE if we can move
 */
SDL_bool checkCharacterColl(Character *a, Character *b, int direction)
{
	if (!a->lives || !b->lives)
		return SDL_TRUE;
	
	// we don't check collision if they're in different dimensions
	if (a->z != b->z)
		return SDL_TRUE;
	
	if (direction == UP && a->y < b->y &&
		(a->x < b->x + 0.8 && a->x > b->x - 0.8) &&
		a->y > b->y - 1.0)
		return SDL_FALSE;
	
	if (direction == DOWN && a->y > b->y &&
		(a->x < b->x + 0.8 && a->x > b->x - 0.8) &&
		a->y < b->y + 1.0)
		return SDL_FALSE;
	
	if (direction == RIGHT && a->x < b->x &&
		(a->y < b->y + 0.8 && a->y > b->y - 0.8) &&
		a->x > b->x - 1.1)
		return SDL_FALSE;
	
	if (direction == LEFT && a->x > b->x &&
		(a->y < b->y + 0.8 && a->y > b->y - 0.8) &&
		a->x < b->x + 1.1)
		return SDL_FALSE;
	
	return SDL_TRUE;
}

/* 
 * Checks if characterA with a given direction is colliding with characterB, characterC, or characterD.
 * Returns SDL_TRUE if the characterA can move without colliding into character B, C, or D; otherwise, it returns SDL_FALSE
 */
SDL_bool checkCharacterCollision(int direction, Character *characterA, Character *characterB, Character *characterC, Character *characterD)
{	
	if (characterA->direction == NO_DIRECTION || !checkCharacterColl(characterA, characterB, direction) || !checkCharacterColl(characterA, characterC, direction) || !checkCharacterColl(characterA, characterD, direction))
		return SDL_FALSE;
	
	return SDL_TRUE;
}

/*
 * Checks if the character will be out of bounds of the checkerboard if it moves within the given direction.
 * Returns SDL_FALSE if character can't move, otherwise returns SDL_TRUE
 */
SDL_bool characterIsOutOfBounds(int direction, Character *character)
{	
	int x;
	int y;
	
	int index = 0;
	
	// trancuate values to integers.
	x = (int)character->x;
	y = (int)character->y;
	
	index = getTileIndexLocation(x, y);
	
	/*
	 * First, check if they're on a tile of which's state is SDL_FALSE
	 * If so, then return SDL_FALSE
	 */
	if (gTiles[index].state == SDL_FALSE)
		return SDL_FALSE;
	
	if (direction == LEFT && (gTiles[index].left == NULL || gTiles[index].left->state == SDL_FALSE || gTiles[index].left->isDead || fabs(gTiles[index].left->red - character->weap->red) < 0.00001))
	{
		/*
		 * We know that the current tile's left tile is destroyed, but we only have access to the current tile.
		 * So we check if the character is < than the current tile's x coordinate - 0.7.
		 * To translate it to a valid value on the checkerboard, in this case, we add the current tile location
		 * by 7.0
		 */
		
		if (character->x < (gTiles[index].x + 7.0) - 0.7)
			return SDL_FALSE;
	}
	
	else if (direction == RIGHT && (gTiles[index].right == NULL || gTiles[index].right->state == SDL_FALSE || gTiles[index].right->isDead || fabs(gTiles[index].right->red - character->weap->red) < 0.00001))
	{
		
		if (character->x > (gTiles[index].x + 7.0) + 0.7)
			return SDL_FALSE;
	}
	
	else if (direction == DOWN && (gTiles[index].down == NULL || gTiles[index].down->state == SDL_FALSE || gTiles[index].down->isDead || fabs(gTiles[index].down->red - character->weap->red) < 0.00001))
	{
		
		if (character->y < ((gTiles[index].y - 18.5) + 6.0) - 0.7)
			return SDL_FALSE;
	}
	
	else if (direction == UP && (gTiles[index].up == NULL || gTiles[index].up->state == SDL_FALSE || gTiles[index].up->isDead || fabs(gTiles[index].up->red - character->weap->red) < 0.00001))
	{
		
		if (character->y > ((gTiles[index].y - 18.5) + 6.0) + 0.7)
			return SDL_FALSE;
	}
	
	return SDL_TRUE;
}

/* x, y arguements are the specified character's x and y values */
int getTileIndexLocation(int x, int y)
{
	int index;
	
	/*
	 * Round x and y values to the tile they're on.
	 * Initially, each tile contains two numbers of values.
	 * Example: tile 2 can have a location of 1.x or 2.x
	 */
	
	switch (x)
	{
		case 0:
			x = 1;
			break;
		case 1 ... 2:
			x = 2;
			break;
		case 3 ... 4:
			x = 3;
			break;
		case 5 ... 6:
			x = 4;
			break;
		case 7 ... 8:
			x = 5;
			break;
		case 9 ... 10:
			x = 6;
			break;
		case 11 ... 12:
			x = 7;
			break;
		case 13 ... 14:
			x = 8;
			break;
	}
	
	switch (y)
	{
		case 0:
			y = 1;
			break;
		case 1 ... 2:
			y = 2;
			break;
		case 3 ... 4:
			y = 3;
			break;
		case 5 ... 6:
			y = 4;
			break;
		case 7 ... 8:
			y = 5;
			break;
		case 9 ... 10:
			y = 6;
			break;
		case 11 ... 12:
			y = 7;
			break;
		case 13 ... 14:
			y = 8;
			break;
	}
	
	// Get the array index of the tile that the character is on
	index = x;
	
	// Find which row the index is on
	if (y != 1)
	{
		index += 8 * (y - 1);
	}
	
	return index;
}
