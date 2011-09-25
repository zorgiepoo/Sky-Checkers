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

#pragma once

#include "maincore.h"

typedef struct
{
	GLfloat x, y, z;
	GLfloat red, green, blue;
	
	SDL_bool drawingState;
	SDL_bool animationState;
	
	// display lists.
	GLuint d_list_right;
	GLuint d_list_left;
	GLuint d_list_up;
	GLuint d_list_down;
	
	int direction;
} Weapon;

void initWeapon(Weapon *weap);

void buildWeaponList(Weapon *weap);
void deleteWeaponList(Weapon *weap);

void drawWeapon(Weapon *weap);
