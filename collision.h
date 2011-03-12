/*
 * Coder: Mayur Pawashe (Zorg)
 * http://zorg.tejat.net
 */

#pragma once

#include "maincore.h"
#include "characters.h"

SDL_bool characterIsOutOfBounds(int direction, Character *character);
int getTileIndexLocation(int x, int y);

SDL_bool checkCharacterColl(Character *a, Character *b, int direction);
SDL_bool checkCharacterCollision(int direction, Character *characterA, Character *characterB, Character *characterC, Character *characterD);
