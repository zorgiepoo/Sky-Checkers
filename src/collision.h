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

SDL_bool characterCanMove(int direction, Character *character);
int getTileIndexLocation(int x, int y);

int rowOfCharacter(Character *character);
int columnOfCharacter(Character *character);

SDL_bool checkCharacterColl(Character *a, Character *b, int direction);
SDL_bool checkCharacterCollision(int direction, Character *characterA, Character *characterB, Character *characterC, Character *characterD);
