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

typedef enum
{
	GAME_STATE_OFF = 0,
	GAME_STATE_ON = 1,
	GAME_STATE_CONNECTING = 2
} GameState;

typedef struct
{
	GameState *gameState;
	SDL_Window *window;
} GameMenuContext;

void initGame(SDL_Window *window, bool firstGame);
void endGame(SDL_Window *window, bool lastGame);

extern bool gGameHasStarted;
extern bool gGameShouldReset;
extern int gGameWinner;
extern int32_t gGameStartNumber;
extern bool gDrawFPS;
extern bool gDrawPings;

extern int gCharacterLives;
extern int gCharacterNetLives;

extern bool gAudioEffectsFlag;
extern bool gAudioMusicFlag;
