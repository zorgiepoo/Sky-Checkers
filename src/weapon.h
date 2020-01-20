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

#pragma once

#include "renderer.h"

#define INITIAL_WEAPON_Z 0.0f

typedef struct
{
	float x, y, z;
	ZGFloat red, green, blue;
	float initialX, initialY;
	float compensation;
	float timeFiring;
	
	bool drawingState;
	bool animationState;
	
	int direction;
	bool fired;
} Weapon;

void initWeapon(Weapon *weap);

void drawWeapon(Renderer *renderer, Weapon *weap);
