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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* macOS */

#ifdef __APPLE__
#ifdef __MACH__
#define MAC_OS_X

#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#import "osx.h"

#endif
#endif

/* Windows */

#ifdef _WIN32
#define WINDOWS

#include "SDL.h"
#include "SDL_syswm.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include "windows.h"

#endif

/* Linux */

#ifdef linux

#include <SDL2/SDL.h>

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "linux.h"

#endif

/* Game Globals */

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

void initGame(SDL_Window *window, SDL_bool firstGame);
void endGame(SDL_Window *window, SDL_bool lastGame);

extern SDL_bool gGameHasStarted;
extern SDL_bool gGameShouldReset;
extern int gGameWinner;
extern int32_t gGameStartNumber;
extern SDL_bool gDrawFPS;
extern SDL_bool gDrawPings;

extern int gCharacterLives;
extern int gCharacterNetLives;

extern SDL_bool gAudioEffectsFlag;
extern SDL_bool gAudioMusicFlag;
