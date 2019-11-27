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

#include "characters.h"
#include "input.h"

#define OBJECT_FALLING_STEP 0.2f

//#define ANIMATION_TIMER_INTERVAL 0.00833 // in seconds
#define ANIMATION_TIMER_INTERVAL 0.01666 // in seconds

void startAnimation(void);
void endAnimation(void);

void animate(SDL_Window *window, double timeDelta);

void prepareCharactersDeath(Character *player);
void decideWhetherToMakeAPlayerAWinner(Character *player);

void recoverDestroyedTile(int tileIndex);
