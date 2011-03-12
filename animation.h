/*
 * Coder: Mayur Pawashe (Zorg)
 * http://zorg.tejat.net
 */

#pragma once

#include "maincore.h"
#include "characters.h"
#include "input.h"

SDL_bool startAnimation(void);
void endAnimation(void);

void prepareCharactersDeath(Character *player);
void decideWhetherToMakeAPlayerAWinner(Character *player);
