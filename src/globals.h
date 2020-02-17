/*
* Copyright 2019 Mayur Pawashe
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

#include "window.h"
#include <stdbool.h>
#include <stdint.h>

#define MAX_CHARACTER_LIVES 10

typedef enum
{
	GAME_STATE_OFF = 0,
	GAME_STATE_ON = 1,
	GAME_STATE_CONNECTING = 2,
	GAME_STATE_PAUSED = 3,
	GAME_STATE_TUTORIAL = 4
} GameState;

typedef struct
{
	GameState *gameState;
	ZGWindow *window;
	void (*exitGame)(ZGWindow *);
} GameMenuContext;

void initGame(ZGWindow *window, bool firstGame, bool tutorial);
void endGame(ZGWindow *window, bool lastGame);

extern bool gPlayedGame;
extern bool gGameHasStarted;
extern bool gGameShouldReset;
extern int gGameWinner;
extern int32_t gGameStartNumber;
extern uint8_t gTutorialStage;
extern float gTutorialCoverTimer;
extern bool gDrawFPS;
extern bool gDrawPings;

extern int gCharacterLives;

extern bool gAudioEffectsFlag;
extern bool gAudioMusicFlag;

#define MAX_SERVER_ADDRESS_SIZE	512
extern char gServerAddressString[MAX_SERVER_ADDRESS_SIZE];
extern int gServerAddressStringIndex;

#define MAX_USER_NAME_SIZE	8
extern char gUserNameString[MAX_USER_NAME_SIZE];
extern int gUserNameStringIndex;
