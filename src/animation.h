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

#include "maincore.h"
#include "characters.h"
#include "input.h"

// If you were to use a stop watch, the time it takes for a character to go from one end
// of the checkerboard to the other end (vertically) is ~3.50-3.60 seconds
#define ANIMATION_TIMER_INTERVAL 0.0177 // in seconds

void startAnimation(void);
void endAnimation(void);

void animate(SDL_Window *window, double timeDelta);

void prepareCharactersDeath(Character *player);
void decideWhetherToMakeAPlayerAWinner(Character *player);
