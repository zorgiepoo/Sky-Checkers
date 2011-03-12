/*
 * Coder: Mayur Pawashe (Zorg)
 * http://zorg.tejat.net
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

// Fullscreen toggle SDL_Event type
#define SDL_FULLSCREEN_TOGGLE	27

/* Mac OS X */

#ifdef __APPLE__
#ifdef __MACH__
#define MAC_OS_X

#include "SDL/SDL.h"
#include "SDL/SDL_opengl.h"
#include "SDL_ttf/SDL_ttf.h"
#include "SDL_image/SDL_image.h"
#include "SDL_mixer/SDL_mixer.h"
#include <GLUT/glut.h>

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

#include "SDL/SDL.h"
#include "SDL/SDL_opengl.h"
#include "SDL/SDL_ttf.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_mixer.h"
#include <GL/glut.h>

#include <winsock.h>
#include "windows.h"

#endif

/* Linux */

#ifdef linux

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "SDL/SDL_mixer.h"
#include "SDL/SDL_ttf.h"
#include "SDL/SDL_image.h"

#include <GL/glut.h>

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

void initGame(void);
void endGame(void);
void closeGameResources(void);

extern SDL_bool gGameHasStarted;
extern SDL_bool gGameShouldReset;
extern int gGameWinner;
extern int gGameStartNumber;
extern SDL_bool gDrawFPS;

extern int gCharacterLives;
extern int gCharacterNetLives;

extern SDL_bool gAudioEffectsFlag;
extern SDL_bool gAudioMusicFlag;

extern SDL_Rect **gResolutions;
extern int gResolutionCounter;
extern int gBestResolutionLimit;

extern int gVsyncFlag;
extern SDL_bool gFpsFlag;
extern SDL_bool gFsaaFlag;
extern SDL_bool gFullscreenFlag;
