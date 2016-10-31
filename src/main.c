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

#include "maincore.h"
#include "characters.h"
#include "scenery.h"
#include "input.h"
#include "console.h"
#include "animation.h"
#include "weapon.h"
#include "font.h"
#include "audio.h"
#include "game_menus.h"
#include "network.h"
#include "utilities.h"

static const int GAME_STATE_OFF =				0;
static const int GAME_STATE_ON =				1;

// in seconds
static const int NEW_GAME_WILL_BEGIN_DELAY =	3;

SDL_bool gGameHasStarted;
SDL_bool gGameShouldReset;
int gGameStartNumber;
int gGameWinner;

SDL_Rect **gResolutions;

Uint32 gVideoFlags;

// Console flag indicating if we can use the console
SDL_bool gConsoleFlag =							SDL_FALSE;

// audio flags
SDL_bool gAudioEffectsFlag =					SDL_TRUE;
SDL_bool gAudioMusicFlag =						SDL_TRUE;

// video flags
int gVsyncFlag =								1;
SDL_bool gFpsFlag =								SDL_FALSE;
SDL_bool gFsaaFlag =							SDL_TRUE;
SDL_bool gFullscreenFlag =						SDL_FALSE;


SDL_bool gValidDefaults =						SDL_FALSE;

// video counters
int gResolutionCounter =						-1;

int gBestResolutionLimit =						-1;

// Screen gResolutions
int gScreenWidth =								0;
int gScreenHeight =								0;

// Lives
int gCharacterLives =							5;
int gCharacterNetLives =						5;

// Fullscreen video state.
SDL_bool gFullscreen =							SDL_FALSE;

static SDL_Surface *gScreen =					NULL;

static SDL_bool gConsoleActivated =				SDL_FALSE;
SDL_bool gDrawFPS =								SDL_FALSE;

static int gGameState;
static SDL_bool gGameLaunched =					SDL_FALSE;

void initGame(void);

static void createSurface(Uint32 flags);
static void resizeWindow(int width, int height);

static int compareResolutions(SDL_Rect rectOne, SDL_Rect rectTwo);
#define MIN_SCREEN_RESOLUTION_WIDTH				640
#define MIN_SCREEN_RESOLUTION_HEIGHT			480
static void killResolutionsBelow(int width, int height);

static void initScene(void);

static void readDefaults(void);
static void writeDefaults(void);

static void initGL(void)
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	// initialize random number generator
	mt_init();

	initScene();

	/*
	 * Load a few font strings before a game starts up.
	 * We don't want the user to experience slow font loading times during a game
	 */
	drawString(40.0, 10.0, "Game begins in 1");
	drawString(40.0, 10.0, "Game begins in 2");
	drawString(40.0, 10.0, "Game begins in 3");
	drawString(40.0, 10.0, "Game begins in 4");
	drawString(40.0, 10.0, "Game begins in 5");

	drawString(0.5, 0.5, "Wins:");
	drawString(0.5, 0.5, "Kills:");
}

static void drawBlackBox(void)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	
	GLfloat vertices[] =
	{
		-17.3f, 17.3f, -13.0f,
		17.3f, 17.3f, -13.0f,
		17.3f, -17.3f, -13.0f,
		-17.3f, -17.3f, -13.0f,
	};

	glVertexPointer(3, GL_FLOAT, 0, vertices);
	glDrawArrays(GL_QUADS, 0, 4);

	glDisableClientState(GL_VERTEX_ARRAY);
}

static void initScene(void)
{
	loadSceneryTextures();

	initTiles();

	initCharacters();

	loadCharacterTextures();
	buildCharacterLists();

	// build weapon lists
	buildWeaponList(gRedRover.weap);
	buildWeaponList(gGreenTree.weap);
	buildWeaponList(gPinkBubbleGum.weap);
	buildWeaponList(gBlueLightning.weap);

	// defaults couldn't be read
	if (!gValidDefaults)
	{
		// init the inputs
		initInput(&gRedRoverInput, SDLK_b, SDLK_c, SDLK_f, SDLK_v, SDLK_g);
		initInput(&gGreenTreeInput, SDLK_l, SDLK_j, SDLK_i, SDLK_k, SDLK_m);
		initInput(&gBlueLightningInput, SDLK_d, SDLK_a, SDLK_w, SDLK_s, SDLK_z);
		initInput(&gPinkBubbleGumInput, SDLK_RIGHT, SDLK_LEFT, SDLK_UP, SDLK_DOWN, SDLK_SPACE);

		// init character states
		gPinkBubbleGum.state = CHARACTER_HUMAN_STATE;
		gRedRover.state = CHARACTER_AI_STATE;
		gGreenTree.state = CHARACTER_AI_STATE;
		gBlueLightning.state = CHARACTER_AI_STATE;

		gAIMode = AI_EASY_MODE;
	}

#ifdef _DEBUG
	gConsoleFlag = SDL_TRUE;
#endif

	gRedRoverInput.character = &gRedRover;
	gGreenTreeInput.character = &gGreenTree;
	gPinkBubbleGumInput.character = &gPinkBubbleGum;
	gBlueLightningInput.character = &gBlueLightning;

	initConsole();

	gGameState = GAME_STATE_OFF;

	initMenus();
}

static void readDefaults(void)
{
	#ifndef WINDOWS
		// this would be a good time to get the default user name
		getDefaultUserName(gUserNameString, MAX_USER_NAME_SIZE - 1);
		gUserNameStringIndex = strlen(gUserNameString);
	#endif

	FILE *fp = getUserDataFile("r");

	if (fp == NULL)
	{
		return;
	}

	gValidDefaults = SDL_TRUE;

	// conole flag default
	fscanf(fp, "Console flag: %i\n\n", (int *)&gConsoleFlag);

	// video defaults
	fscanf(fp, "screen width: %i\n", &gScreenWidth);
	fscanf(fp, "screen height: %i\n", &gScreenHeight);
	fscanf(fp, "vsync flag: %i\n", &gVsyncFlag);

	// typecasting (int *) seems to work for SDL_bool pointers
	fscanf(fp, "fps flag: %i\n", (int *)&gFpsFlag);
	fscanf(fp, "FSAA flag: %i\n", (int *)&gFsaaFlag);

	fscanf(fp, "Fullscreen flag: %i\n", (int *)&gFullscreenFlag);

	fscanf(fp, "\n");

	fscanf(fp, "Number of lives: %i\n", &gCharacterLives);
	fscanf(fp, "Number of net lives: %i\n", &gCharacterNetLives);

	fscanf(fp, "\n");

	// character states
	fscanf(fp, "Pink Bubblegum state: %i\n", &gPinkBubbleGum.state);
	fscanf(fp, "Red Rover state: %i\n", &gRedRover.state);
	fscanf(fp, "Green Tree state: %i\n", &gGreenTree.state);
	fscanf(fp, "Blue Lightning state: %i\n", &gBlueLightning.state);

	fscanf(fp, "AI Mode: %i\n", &gAIMode);
	fscanf(fp, "AI Net Mode: %i\n", &gAINetMode);
	fscanf(fp, "Number of Net Humans: %i\n", &gNumberOfNetHumans);

	fscanf(fp, "\n");

	// PinkBubbleGum defaults
	fscanf(fp, "Pink Bubblegum key right: %i\n", &gPinkBubbleGumInput.r_id);
	fscanf(fp, "Pink Bubblegum key left: %i\n", &gPinkBubbleGumInput.l_id);
	fscanf(fp, "Pink Bubblegum key up: %i\n", &gPinkBubbleGumInput.u_id);
	fscanf(fp, "Pink Bubblegum key down: %i\n", &gPinkBubbleGumInput.d_id);
	fscanf(fp, "Pink Bubblegum key weapon: %i\n", &gPinkBubbleGumInput.weap_id);

	fscanf(fp, "\n");

	gPinkBubbleGumInput.joy_right = malloc(30);
	gPinkBubbleGumInput.joy_left = malloc(30);
	gPinkBubbleGumInput.joy_up = malloc(30);
	gPinkBubbleGumInput.joy_down = malloc(30);
	gPinkBubbleGumInput.joy_weap = malloc(30);

	// %s will not work correctly, but %[A-Za-z0-9 ] does
	fscanf(fp, "Pink Bubblegum joy right id: %i, axis: %i, joy id: %i (%[A-Za-z0-9 ])\n", &gPinkBubbleGumInput.rjs_id, &gPinkBubbleGumInput.rjs_axis_id, &gPinkBubbleGumInput.joy_right_id, gPinkBubbleGumInput.joy_right);
	fscanf(fp, "Pink Bubblegum joy left id: %i, axis: %i, joy id: %i (%[A-Za-z0-9 ])\n", &gPinkBubbleGumInput.ljs_id, &gPinkBubbleGumInput.ljs_axis_id, &gPinkBubbleGumInput.joy_left_id, gPinkBubbleGumInput.joy_left);
	fscanf(fp, "Pink Bubblegum joy up id: %i, axis: %i, joy id: %i (%[A-Za-z0-9 ])\n", &gPinkBubbleGumInput.ujs_id, &gPinkBubbleGumInput.ujs_axis_id, &gPinkBubbleGumInput.joy_up_id, gPinkBubbleGumInput.joy_up);
	fscanf(fp, "Pink Bubblegum joy down id: %i, axis: %i, joy id: %i (%[A-Za-z0-9 ])\n", &gPinkBubbleGumInput.djs_id, &gPinkBubbleGumInput.djs_axis_id, &gPinkBubbleGumInput.joy_down_id, gPinkBubbleGumInput.joy_down);
	fscanf(fp, "Pink Bubblegum joy weapon id: %i, axis: %i, joy id: %i (%[A-Za-z0-9 ])\n", &gPinkBubbleGumInput.weapjs_id, &gPinkBubbleGumInput.weapjs_axis_id, &gPinkBubbleGumInput.joy_weap_id, gPinkBubbleGumInput.joy_weap);

	fscanf(fp, "\n");

	// RedRover defaults
	fscanf(fp, "Red Rover key right: %i\n", &gRedRoverInput.r_id);
	fscanf(fp, "Red Rover key left: %i\n", &gRedRoverInput.l_id);
	fscanf(fp, "Red Rover key up: %i\n", &gRedRoverInput.u_id);
	fscanf(fp, "Red Rover key down: %i\n", &gRedRoverInput.d_id);
	fscanf(fp, "Red Rover key weapon: %i\n", &gRedRoverInput.weap_id);

	fscanf(fp, "\n");

	gRedRoverInput.joy_right = malloc(30);
	gRedRoverInput.joy_left = malloc(30);
	gRedRoverInput.joy_up = malloc(30);
	gRedRoverInput.joy_down = malloc(30);
	gRedRoverInput.joy_weap = malloc(30);

	fscanf(fp, "Red Rover joy right id: %i, axis: %i, joy id: %i (%[A-Za-z0-9 ])\n", &gRedRoverInput.rjs_id, &gRedRoverInput.rjs_axis_id, &gRedRoverInput.joy_right_id, gRedRoverInput.joy_right);
	fscanf(fp, "Red Rover joy left id: %i, axis: %i, joy id: %i (%[A-Za-z0-9 ])\n", &gRedRoverInput.ljs_id, &gRedRoverInput.ljs_axis_id, &gRedRoverInput.joy_left_id, gRedRoverInput.joy_left);
	fscanf(fp, "Red Rover joy up id: %i, axis: %i, joy id: %i (%[A-Za-z0-9 ])\n", &gRedRoverInput.ujs_id, &gRedRoverInput.ujs_axis_id, &gRedRoverInput.joy_up_id, gRedRoverInput.joy_up);
	fscanf(fp, "Red Rover joy down id: %i, axis: %i, joy id: %i (%[A-Za-z0-9 ])\n", &gRedRoverInput.djs_id, &gRedRoverInput.djs_axis_id, &gRedRoverInput.joy_down_id, gRedRoverInput.joy_down);
	fscanf(fp, "Red Rover joy weapon id: %i, axis: %i, joy id: %i (%[A-Za-z0-9 ])\n", &gRedRoverInput.weapjs_id, &gRedRoverInput.weapjs_axis_id, &gRedRoverInput.joy_weap_id, gRedRoverInput.joy_weap);

	fscanf(fp, "\n");

	// GreenTree defaults
	fscanf(fp, "Green Tree key right: %i\n", &gGreenTreeInput.r_id);
	fscanf(fp, "Green Tree key left: %i\n", &gGreenTreeInput.l_id);
	fscanf(fp, "Green Tree key up: %i\n", &gGreenTreeInput.u_id);
	fscanf(fp, "Green Tree key down: %i\n", &gGreenTreeInput.d_id);
	fscanf(fp, "Green Tree key weapon: %i\n", &gGreenTreeInput.weap_id);

	fscanf(fp, "\n");

	gGreenTreeInput.joy_right = malloc(30);
	gGreenTreeInput.joy_left = malloc(30);
	gGreenTreeInput.joy_up = malloc(30);
	gGreenTreeInput.joy_down = malloc(30);
	gGreenTreeInput.joy_weap = malloc(30);

	fscanf(fp, "Green Tree joy right id: %i, axis: %i, joy id: %i (%[A-Za-z0-9 ])\n", &gGreenTreeInput.rjs_id, &gGreenTreeInput.rjs_axis_id, &gGreenTreeInput.joy_right_id, gGreenTreeInput.joy_right);
	fscanf(fp, "Green Tree joy left id: %i, axis: %i, joy id: %i (%[A-Za-z0-9 ])\n", &gGreenTreeInput.ljs_id, &gGreenTreeInput.ljs_axis_id, &gGreenTreeInput.joy_left_id, gGreenTreeInput.joy_left);
	fscanf(fp, "Green Tree joy up id: %i, axis: %i, joy id: %i (%[A-Za-z0-9 ])\n", &gGreenTreeInput.ujs_id, &gGreenTreeInput.ujs_axis_id, &gGreenTreeInput.joy_up_id, gGreenTreeInput.joy_up);
	fscanf(fp, "Green Tree joy down id: %i, axis: %i, joy id: %i (%[A-Za-z0-9 ])\n", &gGreenTreeInput.djs_id, &gGreenTreeInput.djs_axis_id, &gGreenTreeInput.joy_down_id, gGreenTreeInput.joy_down);
	fscanf(fp, "Green Tree joy weapon id: %i, axis: %i, joy id: %i (%[A-Za-z0-9 ])\n", &gGreenTreeInput.weapjs_id, &gGreenTreeInput.weapjs_axis_id, &gGreenTreeInput.joy_weap_id, gGreenTreeInput.joy_weap);

	fscanf(fp, "\n");

	// BlueLightning defaults
	fscanf(fp, "Blue Lightning key right: %i\n", &gBlueLightningInput.r_id);
	fscanf(fp, "Blue Lightning key left: %i\n", &gBlueLightningInput.l_id);
	fscanf(fp, "Blue Lightning key up: %i\n", &gBlueLightningInput.u_id);
	fscanf(fp, "Blue Lightning key down: %i\n", &gBlueLightningInput.d_id);
	fscanf(fp, "Blue Lightning key weapon: %i\n", &gBlueLightningInput.weap_id);

	fscanf(fp, "\n");

	gBlueLightningInput.joy_right = malloc(30);
	gBlueLightningInput.joy_left = malloc(30);
	gBlueLightningInput.joy_up = malloc(30);
	gBlueLightningInput.joy_down = malloc(30);
	gBlueLightningInput.joy_weap = malloc(30);

	fscanf(fp, "Blue Lightning joy right id: %i, axis: %i, joy id: %i (%[A-Za-z0-9 ])\n", &gBlueLightningInput.rjs_id, &gBlueLightningInput.rjs_axis_id, &gBlueLightningInput.joy_right_id, gBlueLightningInput.joy_right);
	fscanf(fp, "Blue Lightning joy left id: %i, axis: %i, joy id: %i (%[A-Za-z0-9 ])\n", &gBlueLightningInput.ljs_id, &gBlueLightningInput.ljs_axis_id, &gBlueLightningInput.joy_left_id, gBlueLightningInput.joy_left);
	fscanf(fp, "Blue Lightning joy up id: %i, axis: %i, joy id: %i (%[A-Za-z0-9 ])\n", &gBlueLightningInput.ujs_id, &gBlueLightningInput.ujs_axis_id, &gBlueLightningInput.joy_up_id, gBlueLightningInput.joy_up);
	fscanf(fp, "Blue Lightning joy down id: %i, axis: %i, joy id: %i (%[A-Za-z0-9 ])\n", &gBlueLightningInput.djs_id, &gBlueLightningInput.djs_axis_id, &gBlueLightningInput.joy_down_id, gBlueLightningInput.joy_down);
	fscanf(fp, "Blue Lightning joy weapon id: %i, axis: %i, joy id: %i (%[A-Za-z0-9 ])\n", &gBlueLightningInput.weapjs_id, &gBlueLightningInput.weapjs_axis_id, &gBlueLightningInput.joy_weap_id, gBlueLightningInput.joy_weap);

	fscanf(fp, "\n");
	fscanf(fp, "Server IP Address: %s\n", gServerAddressString);
	gServerAddressStringIndex = strlen(gServerAddressString);

	fscanf(fp, "\n");
	fscanf(fp, "Net name: %s\n", gUserNameString);
	gUserNameStringIndex = strlen(gUserNameString);
	
	fscanf(fp, "\n");
	fscanf(fp, "Audio effects: %i\n", (int *)&gAudioEffectsFlag);
	fscanf(fp, "Music: %i\n", (int *)&gAudioMusicFlag);

	fclose(fp);
}

static void writeDefaults(void)
{
	FILE *fp = getUserDataFile("w");

	if (fp == NULL)
	{
		zgPrint("writeDefaults: file pointer is NULL");
		return;
	}

	// conole flag default
#ifdef _DEBUG
	gConsoleFlag = SDL_FALSE;
#endif
	fprintf(fp, "Console flag: %i\n\n", gConsoleFlag);

	// video defaults
	fprintf(fp, "screen width: %i\n", gScreenWidth);
	fprintf(fp, "screen height: %i\n", gScreenHeight);
	fprintf(fp, "vsync flag: %i\n", gVsyncFlag);
	fprintf(fp, "fps flag: %i\n", gFpsFlag);
	fprintf(fp, "FSAA flag: %i\n", gFsaaFlag);
	fprintf(fp, "Fullscreen flag: %i\n", gFullscreenFlag);

	fprintf(fp, "\n");

	fprintf(fp, "Number of lives: %i\n", gCharacterLives);
	fprintf(fp, "Number of net lives: %i\n", gCharacterNetLives);

	fprintf(fp, "\n");

	// character states
	fprintf(fp, "Pink Bubblegum state: %i\n", offlineCharacterState(&gPinkBubbleGum));
	fprintf(fp, "Red Rover state: %i\n", offlineCharacterState(&gRedRover));
	fprintf(fp, "Green Tree state: %i\n", offlineCharacterState(&gGreenTree));
	fprintf(fp, "Blue Lightning state: %i\n", offlineCharacterState(&gBlueLightning));

	fprintf(fp, "AI Mode: %i\n", gAIMode);
	fprintf(fp, "AI Net Mode: %i\n", gAINetMode);
	fprintf(fp, "Number of Net Humans: %i\n", gNumberOfNetHumans);

	fprintf(fp, "\n");

	// PinkBubbleGum defaults
	fprintf(fp, "Pink Bubblegum key right: %i\n", gPinkBubbleGumInput.r_id);
	fprintf(fp, "Pink Bubblegum key left: %i\n", gPinkBubbleGumInput.l_id);
	fprintf(fp, "Pink Bubblegum key up: %i\n", gPinkBubbleGumInput.u_id);
	fprintf(fp, "Pink Bubblegum key down: %i\n", gPinkBubbleGumInput.d_id);
	fprintf(fp, "Pink Bubblegum key weapon: %i\n", gPinkBubbleGumInput.weap_id);

	fprintf(fp, "\n");

	fprintf(fp, "Pink Bubblegum joy right id: %i, axis: %i, joy id: %i (%s)\n", gPinkBubbleGumInput.rjs_id, gPinkBubbleGumInput.rjs_axis_id, gPinkBubbleGumInput.joy_right_id, gPinkBubbleGumInput.joy_right);
	fprintf(fp, "Pink Bubblegum joy left id: %i, axis: %i, joy id: %i (%s)\n", gPinkBubbleGumInput.ljs_id, gPinkBubbleGumInput.ljs_axis_id, gPinkBubbleGumInput.joy_left_id, gPinkBubbleGumInput.joy_left);
	fprintf(fp, "Pink Bubblegum joy up id: %i, axis: %i, joy id: %i (%s)\n", gPinkBubbleGumInput.ujs_id, gPinkBubbleGumInput.ujs_axis_id, gPinkBubbleGumInput.joy_up_id, gPinkBubbleGumInput.joy_up);
	fprintf(fp, "Pink Bubblegum joy down id: %i, axis: %i, joy id: %i (%s)\n", gPinkBubbleGumInput.djs_id, gPinkBubbleGumInput.djs_axis_id, gPinkBubbleGumInput.joy_down_id, gPinkBubbleGumInput.joy_down);
	fprintf(fp, "Pink Bubblegum joy weapon id: %i, axis: %i, joy id: %i (%s)\n", gPinkBubbleGumInput.weapjs_id, gPinkBubbleGumInput.weapjs_axis_id, gPinkBubbleGumInput.joy_weap_id, gPinkBubbleGumInput.joy_weap);

	fprintf(fp, "\n");

	// RedRover defaults
	fprintf(fp, "Red Rover key right: %i\n", gRedRoverInput.r_id);
	fprintf(fp, "Red Rover key left: %i\n", gRedRoverInput.l_id);
	fprintf(fp, "Red Rover key up: %i\n", gRedRoverInput.u_id);
	fprintf(fp, "Red Rover key down: %i\n", gRedRoverInput.d_id);
	fprintf(fp, "Red Rover key weapon: %i\n", gRedRoverInput.weap_id);

	fprintf(fp, "\n");

	fprintf(fp, "Red Rover joy right id: %i, axis: %i, joy id: %i (%s)\n", gRedRoverInput.rjs_id, gRedRoverInput.rjs_axis_id, gRedRoverInput.joy_right_id, gRedRoverInput.joy_right);
	fprintf(fp, "Red Rover joy left id: %i, axis: %i, joy id: %i (%s)\n", gRedRoverInput.ljs_id, gRedRoverInput.ljs_axis_id, gRedRoverInput.joy_left_id, gRedRoverInput.joy_left);
	fprintf(fp, "Red Rover joy up id: %i, axis: %i, joy id: %i (%s)\n", gRedRoverInput.ujs_id, gRedRoverInput.ujs_axis_id, gRedRoverInput.joy_up_id, gRedRoverInput.joy_up);
	fprintf(fp, "Red Rover joy down id: %i, axis: %i, joy id: %i (%s)\n", gRedRoverInput.djs_id, gRedRoverInput.djs_axis_id, gRedRoverInput.joy_down_id, gRedRoverInput.joy_down);
	fprintf(fp, "Red Rover joy weapon id: %i, axis: %i, joy id: %i (%s)\n", gRedRoverInput.weapjs_id, gRedRoverInput.weapjs_axis_id, gRedRoverInput.joy_weap_id, gRedRoverInput.joy_weap);

	fprintf(fp, "\n");

	// GreenTree defaults
	fprintf(fp, "Green Tree key right: %i\n", gGreenTreeInput.r_id);
	fprintf(fp, "Green Tree key left: %i\n", gGreenTreeInput.l_id);
	fprintf(fp, "Green Tree key up: %i\n", gGreenTreeInput.u_id);
	fprintf(fp, "Green Tree key down: %i\n", gGreenTreeInput.d_id);
	fprintf(fp, "Green Tree key weapon: %i\n", gGreenTreeInput.weap_id);

	fprintf(fp, "\n");

	fprintf(fp, "Green Tree joy right id: %i, axis: %i, joy id: %i (%s)\n", gGreenTreeInput.rjs_id, gGreenTreeInput.rjs_axis_id, gGreenTreeInput.joy_right_id, gGreenTreeInput.joy_right);
	fprintf(fp, "Green Tree joy left id: %i, axis: %i, joy id: %i (%s)\n", gGreenTreeInput.ljs_id, gGreenTreeInput.ljs_axis_id, gGreenTreeInput.joy_left_id, gGreenTreeInput.joy_left);
	fprintf(fp, "Green Tree joy up id: %i, axis: %i, joy id: %i (%s)\n", gGreenTreeInput.ujs_id, gGreenTreeInput.ujs_axis_id, gGreenTreeInput.joy_up_id, gGreenTreeInput.joy_up);
	fprintf(fp, "Green Tree joy down id: %i, axis: %i, joy id: %i (%s)\n", gGreenTreeInput.djs_id, gGreenTreeInput.djs_axis_id, gGreenTreeInput.joy_down_id, gGreenTreeInput.joy_down);
	fprintf(fp, "Green Tree joy weapon id: %i, axis: %i, joy id: %i (%s)\n", gGreenTreeInput.weapjs_id, gGreenTreeInput.weapjs_axis_id, gGreenTreeInput.joy_weap_id, gGreenTreeInput.joy_weap);

	fprintf(fp, "\n");

	// BlueLightning defaults
	fprintf(fp, "Blue Lightning key right: %i\n", gBlueLightningInput.r_id);
	fprintf(fp, "Blue Lightning key left: %i\n", gBlueLightningInput.l_id);
	fprintf(fp, "Blue Lightning key up: %i\n", gBlueLightningInput.u_id);
	fprintf(fp, "Blue Lightning key down: %i\n", gBlueLightningInput.d_id);
	fprintf(fp, "Blue Lightning key weapon: %i\n", gBlueLightningInput.weap_id);

	fprintf(fp, "\n");

	fprintf(fp, "Blue Lightning joy right id: %i, axis: %i, joy id: %i (%s)\n", gBlueLightningInput.rjs_id, gBlueLightningInput.rjs_axis_id, gBlueLightningInput.joy_right_id, gBlueLightningInput.joy_right);
	fprintf(fp, "Blue Lightning joy left id: %i, axis: %i, joy id: %i (%s)\n", gBlueLightningInput.ljs_id, gBlueLightningInput.ljs_axis_id, gBlueLightningInput.joy_left_id, gBlueLightningInput.joy_left);
	fprintf(fp, "Blue Lightning joy up id: %i, axis: %i, joy id: %i (%s)\n", gBlueLightningInput.ujs_id, gBlueLightningInput.ujs_axis_id, gBlueLightningInput.joy_up_id, gBlueLightningInput.joy_up);
	fprintf(fp, "Blue Lightning joy down id: %i, axis: %i, joy id: %i (%s)\n", gBlueLightningInput.djs_id, gBlueLightningInput.djs_axis_id, gBlueLightningInput.joy_down_id, gBlueLightningInput.joy_down);
	fprintf(fp, "Blue Lightning joy weapon id: %i, axis: %i, joy id: %i (%s)\n", gBlueLightningInput.weapjs_id, gBlueLightningInput.weapjs_axis_id, gBlueLightningInput.joy_weap_id, gBlueLightningInput.joy_weap);

	fprintf(fp, "\n");
	fprintf(fp, "Server IP Address: %s\n", gServerAddressString);

	fprintf(fp, "\n");
	fprintf(fp, "Net name: %s\n", gUserNameString);
	
	fprintf(fp, "\n");
	fprintf(fp, "Audio effects: %i\n", gAudioEffectsFlag);
	fprintf(fp, "Music: %i\n", gAudioMusicFlag);

	fclose(fp);
}

void initGame(void)
{
	loadTiles();

	loadCharacter(&gRedRover);
	loadCharacter(&gGreenTree);
	loadCharacter(&gPinkBubbleGum);
	loadCharacter(&gBlueLightning);

	if (!startAnimation())
	{
		zgPrint("Timer is long gone");
	}

	int initialNumberOfLives = (gNetworkConnection && gNetworkConnection->type == NETWORK_CLIENT_TYPE) ? gNetworkConnection->characterLives : (gNetworkConnection && gNetworkConnection->type == NETWORK_SERVER_TYPE ? gCharacterNetLives : gCharacterLives);

	gRedRover.lives = initialNumberOfLives;
	gGreenTree.lives = initialNumberOfLives;
	gPinkBubbleGum.lives = initialNumberOfLives;
	gBlueLightning.lives = initialNumberOfLives;

	gGameState = GAME_STATE_ON;

	// wait for NEW_GAME_WILL_BEGIN_DELAY seconds until the game can be started.
	gGameStartNumber = NEW_GAME_WILL_BEGIN_DELAY;

	if (!gConsoleActivated)
	{
		SDL_EnableKeyRepeat(1, SDL_DEFAULT_REPEAT_INTERVAL);
	}
}

void endGame(void)
{
	gGameHasStarted = SDL_FALSE;

	endAnimation();

	gGameState = GAME_STATE_OFF;

	if (!gConsoleActivated)
	{
		SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);
	}

	gGameWinner = NO_CHARACTER;
	gGameShouldReset = SDL_FALSE;
}

void closeGameResources(void)
{
	gRedRover.wins = 0;
	gPinkBubbleGum.wins = 0;
	gGreenTree.wins = 0;
	gBlueLightning.wins = 0;
}

// frames per second function.
void drawFramesPerSecond(void)
{
	static unsigned frame_count = 0;
    static double last_fps_time = -1.0;
    static double last_frame_time = -1.0;
	double now;
	double dt_fps;
	int len, i;

    if (last_frame_time < 0.0)
	{
		last_frame_time = SDL_GetTicks() / 1000.0;
        last_fps_time = last_frame_time;
    }

	now = SDL_GetTicks() / 1000.0;
    last_frame_time = now;
    dt_fps = now - last_fps_time;

	static char fpsString[128];

    if (dt_fps > 1.0)
	{
		sprintf(fpsString, "fps: %0.2f", frame_count / dt_fps);

        frame_count = 0;
        last_fps_time = now;
    }

	frame_count++;

	// Draw the string
	glColor3f(0.0f, 0.5f, 0.8f);
	glRasterPos4f(6.5f, 9.3f, -25.0f, 2.0f);

	len = (int)strlen(fpsString);

	for (i = 0; i < len; i++)
	{
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, fpsString[i]);
	}
}

static void drawScene(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	if (gGameState)
	{
		// weapons.
		drawWeapon(gRedRover.weap);
		drawWeapon(gGreenTree.weap);
		drawWeapon(gPinkBubbleGum.weap);
		drawWeapon(gBlueLightning.weap);

		drawCharacter(&gRedRover);
		drawCharacter(&gGreenTree);
		drawCharacter(&gPinkBubbleGum);
		drawCharacter(&gBlueLightning);

		drawTiles();

		if (gDrawFPS)
		{
			drawFramesPerSecond();
		}

		if (!gGameHasStarted)
		{
			glColor4f(0.0, 0.0, 1.0, 0.5);
			glTranslatef(-1.0, 80.0, -280.0);

			if (gPinkBubbleGum.netState == NETWORK_PENDING_STATE || gRedRover.netState == NETWORK_PENDING_STATE || gGreenTree.netState == NETWORK_PENDING_STATE || gBlueLightning.netState == NETWORK_PENDING_STATE)
			{
				if (gNetworkConnection)
				{
					// be sure to take account the plural form of player(s)
					if (gNetworkConnection->numberOfPlayersToWaitFor > 1)
					{
						drawStringf(50.0, 10.0, "Waiting for %i players to connect...", gNetworkConnection->numberOfPlayersToWaitFor);
					}
					else if (gNetworkConnection->numberOfPlayersToWaitFor == 0)
					{
						drawString(50.0, 10.0, "Waiting for players to connect...");
					}
					else
					{
						drawString(50.0, 10.0, "Waiting for 1 player to connect...");
					}
				}
			}

			else if (gGameStartNumber > 0)
				drawStringf(40.0, 10.0, "Game begins in %i", gGameStartNumber);

			else if (gGameStartNumber == 0)
			{
				gGameHasStarted = SDL_TRUE;
			}

			glLoadIdentity();
		}
	}

	// For this blending to work properly, we have to draw the sky *right* here.

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	drawSky();

	glDisable(GL_BLEND);

	if (gGameState)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		drawCharacterIcons();

		glDisable(GL_BLEND);

		drawCharacterLives();

		if (gConsoleActivated)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			drawConsoleText();
			drawConsole();

			glDisable(GL_BLEND);
		}

		if (gGameWinner != NO_CHARACTER)
		{
			glTranslatef(70.0f, 100.0f, -280.0f);
			glColor4f(1.0f, 1.0f, 1.0f, 0.8f);

			if (gNetworkConnection)
			{
				if (gGameWinner == IDOfCharacter(gNetworkConnection->input->character))
				{
					drawString(20.0, 10.0, "You win!");
				}
				else
				{
					drawString(20.0, 10.0, "You lose!");
				}
			}
			else
			{
				if (gGameWinner == RED_ROVER)
				{
					drawString(40.0, 10.0, "Red Rover wins!");
				}
				else if (gGameWinner == GREEN_TREE)
				{
					drawString(40.0, 10.0, "Green Tree wins!");
				}
				else if (gGameWinner == PINK_BUBBLE_GUM)
				{
					drawString(40.0, 10.0, "Pink Bubblegum wins!");
				}
				else if (gGameWinner == BLUE_LIGHTNING)
				{
					drawString(40.0, 10.0, "Blue Lightning wins!");
				}
			}

			glLoadIdentity();

			/* Stats */

			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// Draw black box
			glTranslatef(0.0, 0.0, -25.0);

			glColor4f(0.0, 0.0, 0.0, 0.5);

			drawBlackBox();

			glLoadIdentity();

			glEnable(GL_TEXTURE_2D);

			/* Display stats */

			// pinkBubbleGum
			glTranslatef(-6.0, 7.0, -25.0);

			drawPinkBubbleGumIcon();

			glTranslatef(0.0f, -2.0f, 0.0f);
			drawString(0.5f, 0.5f, "Wins:");

			glRasterPos3f(0.7f, -0.2f, 0.0f);
			drawGLUTStringf("%i", gPinkBubbleGum.wins);

			glTranslatef(0.0f, -2.0f, 0.0f);
			drawString(0.5f, 0.5f, "Kills:");

			glRasterPos3f(0.7f, -0.2f, 0.0f);
			drawGLUTStringf("%i", gPinkBubbleGum.kills);

			glLoadIdentity();

			// redRover
			glEnable(GL_TEXTURE_2D);
			glTranslatef(-2.0f, 7.0f, -25.0f);

			drawRedRoverIcon();

			glTranslatef(0.0f, -2.0f, 0.0f);
			drawString(0.5, 0.5, "Wins:");

			glRasterPos3f(0.7f, -0.2f, 0.0f);
			drawGLUTStringf("%i", gRedRover.wins);

			glTranslatef(0.0f, -2.0f, 0.0f);
			drawString(0.5, 0.5, "Kills:");

			glRasterPos3f(0.7f, -0.2f, 0.0f);
			drawGLUTStringf("%i", gRedRover.kills);

			glLoadIdentity();

			// greenTree
			glEnable(GL_TEXTURE_2D);
			glTranslatef(2.0f, 7.0f, -25.0f);

			drawGreenTreeIcon();

			glTranslatef(0.0f, -2.0f, 0.0f);
			drawString(0.5, 0.5, "Wins:");

			glRasterPos3f(0.7f, -0.2f, 0.0f);
			drawGLUTStringf("%i", gGreenTree.wins);

			glTranslatef(0.0f, -2.0f, 0.0f);
			drawString(0.5, 0.5, "Kills:");

			glRasterPos3f(0.7f, -0.2f, 0.0f);
			drawGLUTStringf("%i", gGreenTree.kills);

			glLoadIdentity();

			// blueLightning
			glEnable(GL_TEXTURE_2D);
			glTranslatef(6.0f, 7.0f, -25.0f);

			drawBlueLightningIcon();

			glTranslatef(0.0f, -2.0f, 0.0f);
			drawString(0.5f, 0.5f, "Wins:");

			glRasterPos3f(0.7f, -0.2f, 0.0f);
			drawGLUTStringf("%i", gBlueLightning.wins);

			glTranslatef(0.0f, -2.0f, 0.0f);
			drawString(0.5, 0.5, "Kills:");

			glRasterPos3f(0.7f, -0.2f, 0.0f);
			drawGLUTStringf("%i", gBlueLightning.kills);

			glLoadIdentity();

			if (!gNetworkConnection || gNetworkConnection->type != NETWORK_CLIENT_TYPE)
			{
				// Draw a "Press ENTER to play again" notice
				glTranslatef(0.0f, -7.0f, -25.0f);
				glColor4f(0.0f, 0.0f, 0.4f, 0.4f);

				drawString(5.0f, 1.0f, "Fire to play again or Escape to quit");

				glLoadIdentity();

				glDisable(GL_BLEND | GL_TEXTURE_2D);
				glEnable(GL_DEPTH_TEST);
			}
		}
	}

	if (!gGameState)
	{
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		/* Draw a black box in the menu screen */
		glTranslatef(0.0f, 0.0f, -25.0f);

		glColor4f(0.0f, 0.0f, 0.0f, 0.7f);

		drawBlackBox();

		glLoadIdentity();

		glColor4f(0.3f, 0.2f, 1.0f, 0.35f);
		glTranslatef(-1.0f, 27.0f, -100.0f);

		drawString(20.0, 5.0, "Sky Checkers");

		glLoadIdentity();

		drawMenus();

		if (isChildBeingDrawn(&gJoyStickConfig[0][1]) /* pinkBubbleGumConfigRightJoyStick */	||
			isChildBeingDrawn(&gJoyStickConfig[1][1]) /* redRoverConfigRightJoyStick */			||
			isChildBeingDrawn(&gJoyStickConfig[2][1]) /* greenTreeRightgJoyStickConfig */		||
			isChildBeingDrawn(&gJoyStickConfig[3][1]) /* blueLightningConfigJoyStick */)
		{
			glTranslatef(-1.0f, 50.0f, -280.0f);

			glColor4f(0.3f, 0.2f, 1.0f, 0.6f);

			glRotatef(45.0f, 1.0f, 0.0f, 0.0f);

			drawString(100.0, 5.0, "Click enter to modify a mapping value and input in a button on your joystick. Click Escape to exit out.");

			glTranslatef(0.0f, -20.0f, 0.0f);

			drawString(50.0, 5.0, "(Joysticks only function in-game)");

			glLoadIdentity();
		}

		else if (isChildBeingDrawn(&gCharacterConfigureKeys[0][1]) /* pinkBubbleGum */	||
				 isChildBeingDrawn(&gCharacterConfigureKeys[1][1]) /* redRover */		||
				 isChildBeingDrawn(&gCharacterConfigureKeys[2][1]) /* greenTree */		||
				 isChildBeingDrawn(&gCharacterConfigureKeys[3][1]) /* blueLightning */)
		{
			glTranslatef(-1.0f, 50.0f, -280.0f);

			glColor4f(0.3f, 0.2f, 1.0f, 0.6f);

			glRotatef(45.0f, 1.0f, 0.0f, 0.0f);

			drawString(100.0, 5.0, "Click enter to modify a mapping value and input in a key. Click Escape to exit out.");

			glLoadIdentity();
		}

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
	}
}

static void eventInput(SDL_Event *event, int *quit)
{
	switch (event->type)
	{
		case SDL_KEYDOWN:

#if defined(WINDOWS) || defined(linux)
			if (event->key.keysym.sym == SDLK_RETURN && ((SDL_GetModState() & KMOD_LALT) || (SDL_GetModState() & KMOD_RALT)))
			{
				event->type = SDL_FULLSCREEN_TOGGLE;
				SDL_PushEvent(event);

				break;
			}
#endif

			if (!gConsoleActivated && gGameState && gGameWinner != NO_CHARACTER &&
				(event->key.keysym.sym == gPinkBubbleGumInput.weap_id || event->key.keysym.sym == gRedRoverInput.weap_id ||
				 event->key.keysym.sym == gBlueLightningInput.weap_id || event->key.keysym.sym == gGreenTreeInput.weap_id ||
				event->key.keysym.sym == SDLK_RETURN || event->key.keysym.sym == SDLK_KP_ENTER))
			{
				// new game
				if (!gNetworkConnection || gNetworkConnection->type != NETWORK_CLIENT_TYPE)
				{
					if (gNetworkConnection && gNetworkConnection->type == NETWORK_SERVER_TYPE)
					{
						sendToClients(0, "ng");
					}
					gGameShouldReset = SDL_TRUE;
				}
			}

			if (event->key.keysym.sym == SDLK_RETURN || event->key.keysym.sym == SDLK_KP_ENTER)
			{
				if (gCurrentMenu == gConfigureLivesMenu)
				{
					gDrawArrowsForCharacterLivesFlag = !gDrawArrowsForCharacterLivesFlag;
					if (gAudioEffectsFlag)
					{
						playMenuSound();
					}
				}

				else if (gCurrentMenu == gScreenResolutionVideoOptionMenu)
				{
					gDrawArrowsForScreenResolutionsFlag = !gDrawArrowsForScreenResolutionsFlag;
					if (gAudioEffectsFlag)
					{
						playMenuSound();
					}
				}

				else if (gCurrentMenu == gRefreshRateVideoOptionMenu)
				{
					gDrawArrowsForRefreshRatesFlag = !gDrawArrowsForRefreshRatesFlag;
					if (gAudioEffectsFlag)
					{
						playMenuSound();
					}
				}

				else if (gCurrentMenu == gAIModeOptionsMenu || gCurrentMenu == gAINetModeOptionsMenu)
				{
					gDrawArrowsForAIModeFlag = !gDrawArrowsForAIModeFlag;
					if (gAudioEffectsFlag)
					{
						playMenuSound();
					}
				}

				else if (!gGameState)
				{
					invokeMenu();
				}

				else if (gConsoleActivated)
				{
					executeConsoleCommand();
				}
			}

			else if (event->key.keysym.sym == SDLK_DOWN)
			{
				if (gNetworkAddressFieldIsActive)
					break;

				if (gDrawArrowsForCharacterLivesFlag)
				{
					if (gCharacterLives == 1)
						gCharacterLives = 10;
					else
						gCharacterLives--;
				}
				else if (gDrawArrowsForNetPlayerLivesFlag)
				{
					if (gCharacterNetLives == 1)
					{
						gCharacterNetLives = 10;
					}
					else
					{
						gCharacterNetLives--;
					}
					
				}
				else if (gDrawArrowsForScreenResolutionsFlag)
				{
					// Resolutions are stored largest to smallest starting from gResolutions[0]
					do
					{
						if (gResolutions[gResolutionCounter + 1])
						{
							gResolutionCounter++;
						}
						else
						{
							gResolutionCounter = gBestResolutionLimit;
						}
					}
					while (gResolutions[gResolutionCounter]->w < gResolutions[gResolutionCounter]->h);
				}
				else if (gDrawArrowsForRefreshRatesFlag)
				{
					if (gFpsFlag)
					{
						gFpsFlag = SDL_FALSE;
					}
					else if (gVsyncFlag)
					{
						gFpsFlag = SDL_TRUE;
						gVsyncFlag = SDL_FALSE;
					}
					else /* if (!gVsyncFlag) */
					{
						gVsyncFlag = SDL_TRUE;
					}

					SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, gVsyncFlag);
				}

				else if (gDrawArrowsForAIModeFlag)
				{
					if (isChildBeingDrawn(gAIModeOptionsMenu))
					{
						if (gAIMode == AI_EASY_MODE)
							gAIMode = AI_HARD_MODE;
						else if (gAIMode == AI_MEDIUM_MODE)
							gAIMode = AI_EASY_MODE;
						else /* if (gAIMode == AI_HARD_MODE) */
							gAIMode = AI_MEDIUM_MODE;
					}
					else if (isChildBeingDrawn(gAINetModeOptionsMenu))
					{
						if (gAINetMode == AI_EASY_MODE)
							gAINetMode = AI_HARD_MODE;
						else if (gAINetMode == AI_MEDIUM_MODE)
							gAINetMode = AI_EASY_MODE;
						else /* if (gAINetMode == AI_HARD_MODE) */
							gAINetMode = AI_MEDIUM_MODE;
					}
				}
				
				else if (gDrawArrowsForNumberOfNetHumansFlag)
				{
					gNumberOfNetHumans--;
					if (gNumberOfNetHumans <= 0)
					{
						gNumberOfNetHumans = 3;
					}
				}

				else if (!gGameState)
				{
					changeMenu(DOWN);

					if (gCurrentMenu == gRedRoverPlayerOptionsMenu && gPinkBubbleGum.state == CHARACTER_AI_STATE)
					{
						changeMenu(DOWN);
					}

					if (gCurrentMenu == gGreenTreePlayerOptionsMenu && gRedRover.state == CHARACTER_AI_STATE)
					{
						changeMenu(DOWN);
					}

					if (gCurrentMenu == gBlueLightningPlayerOptionsMenu && gGreenTree.state == CHARACTER_AI_STATE)
					{
						changeMenu(DOWN);
					}
				}
			}

			else if (event->key.keysym.sym == SDLK_UP)
			{
				if (gNetworkAddressFieldIsActive)
					break;

				if (gDrawArrowsForCharacterLivesFlag)
				{
					if (gCharacterLives == 10)
						gCharacterLives = 1;
					else
						gCharacterLives++;
				}
				else if (gDrawArrowsForNetPlayerLivesFlag)
				{
					if (gCharacterNetLives == 10)
					{
						gCharacterNetLives = 1;
					}
					else
					{
						gCharacterNetLives++;
					}
				}
				else if (gDrawArrowsForScreenResolutionsFlag)
				{
					// Resolutions are stored largest to smallest starting from gResolutions[0]
					do
					{
						if (gResolutionCounter == gBestResolutionLimit)
						{
							int resolutionIndex;
							
							for (resolutionIndex = 0; gResolutions[resolutionIndex]; resolutionIndex++)
								;

							gResolutionCounter = resolutionIndex - 1;
						}
						else
						{
							gResolutionCounter--;
						}
					}
					while (gResolutions[gResolutionCounter]->w < gResolutions[gResolutionCounter]->h);
				}
				else if (gDrawArrowsForRefreshRatesFlag)
				{
					 if (gFpsFlag)
					 {
						 gVsyncFlag = SDL_TRUE;
						 gFpsFlag = SDL_FALSE;
					 }
					 else if (gVsyncFlag)
					 {
						 gVsyncFlag = SDL_FALSE;
					 }
					 else /* if (!gVsyncFlag) */
					 {
						 gFpsFlag = SDL_TRUE;
					 }

					SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, gVsyncFlag);
				}
				else if (gDrawArrowsForAIModeFlag)
				{
					if (isChildBeingDrawn(gAIModeOptionsMenu))
					{
						if (gAIMode == AI_EASY_MODE)
							gAIMode = AI_MEDIUM_MODE;
						else if (gAIMode == AI_MEDIUM_MODE)
							gAIMode = AI_HARD_MODE;
						else /* if (gAIMode == AI_HARD_MODE) */
							gAIMode = AI_EASY_MODE;
					}
					else if (isChildBeingDrawn(gAINetModeOptionsMenu))
					{
						if (gAINetMode == AI_EASY_MODE)
							gAINetMode = AI_MEDIUM_MODE;
						else if (gAINetMode == AI_MEDIUM_MODE)
							gAINetMode = AI_HARD_MODE;
						else /* if (gAINetMode == AI_HARD_MODE) */
							gAINetMode = AI_EASY_MODE;
					}
				}
				else if (gDrawArrowsForNumberOfNetHumansFlag)
				{
					gNumberOfNetHumans++;
					if (gNumberOfNetHumans >= 4)
					{
						gNumberOfNetHumans = 1;
					}
				}

				else if (!gGameState)
				{
					changeMenu(UP);

					if (gCurrentMenu == gBlueLightningPlayerOptionsMenu && gGreenTree.state == CHARACTER_AI_STATE)
					{
						changeMenu(UP);
					}

					if (gCurrentMenu == gGreenTreePlayerOptionsMenu && gRedRover.state == CHARACTER_AI_STATE)
					{
						changeMenu(UP);
					}

					if (gCurrentMenu == gRedRoverPlayerOptionsMenu && gPinkBubbleGum.state == CHARACTER_AI_STATE)
					{
						changeMenu(UP);
					}
				}
			}

			else if (event->key.keysym.sym == SDLK_ESCAPE)
			{
				if (!gGameState)
				{
					if (gNetworkAddressFieldIsActive)
					{
						gNetworkAddressFieldIsActive = SDL_FALSE;
					}
					else if (gNetworkUserNameFieldIsActive)
					{
						gNetworkUserNameFieldIsActive = SDL_FALSE;
					}
					else if (gDrawArrowsForCharacterLivesFlag)
					{
						gDrawArrowsForCharacterLivesFlag = SDL_FALSE;
					}
					else if (gDrawArrowsForNetPlayerLivesFlag)
					{
						gDrawArrowsForNetPlayerLivesFlag = SDL_FALSE;
					}
					else if (gDrawArrowsForScreenResolutionsFlag)
					{
						gDrawArrowsForScreenResolutionsFlag = SDL_FALSE;
					}
					else if (gDrawArrowsForRefreshRatesFlag)
					{
						gDrawArrowsForRefreshRatesFlag = SDL_FALSE;
					}
					else if (gDrawArrowsForAIModeFlag)
					{
						gDrawArrowsForAIModeFlag = SDL_FALSE;
					}
					else if (gDrawArrowsForNumberOfNetHumansFlag)
					{
						gDrawArrowsForNumberOfNetHumansFlag = SDL_FALSE;
					}
					else if (isChildBeingDrawn(gVideoOptionsMenu->mainChild))
					{
						// check to see if the surface needs to be created again
						SDL_bool shouldCreateSurface = SDL_FALSE;
						int value;

						SDL_GL_GetAttribute(SDL_GL_SWAP_CONTROL, &value);

						if (value != gVsyncFlag)
						{
							shouldCreateSurface = SDL_TRUE;
						}

						SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &value);

						if (value != (int)gFsaaFlag)
						{
							shouldCreateSurface = SDL_TRUE;
						}

						if (gScreenWidth != gResolutions[gResolutionCounter]->w || gScreenHeight != gResolutions[gResolutionCounter]->h)
						{
							// set the new size
							gScreenWidth = gResolutions[gResolutionCounter]->w;
							gScreenHeight = gResolutions[gResolutionCounter]->h;

							shouldCreateSurface = SDL_TRUE;
						}

						if (gFullscreenFlag != gFullscreen)
						{
							event->type = SDL_FULLSCREEN_TOGGLE;
							SDL_PushEvent(event);
						}

						if (shouldCreateSurface)
						{
							createSurface(gVideoFlags);
							// we need to fetch the current list of resolutions again, otherwise, SDL screws up the resolutions
							gResolutions = SDL_ListModes(NULL, SDL_FULLSCREEN | SDL_HWSURFACE);
							killResolutionsBelow(MIN_SCREEN_RESOLUTION_WIDTH, MIN_SCREEN_RESOLUTION_HEIGHT);
						}

						changeMenu(LEFT);
					}
					else
					{
						changeMenu(LEFT);
					}
				}
				else /* if (gGameState) */
				{
					if (!gConsoleActivated)
					{
						endGame();
						closeGameResources();

						if (gNetworkConnection)
						{
							if (gNetworkConnection->type == NETWORK_SERVER_TYPE)
							{
								sendToClients(0, "qu");
							}
							else if (gNetworkConnection->type == NETWORK_CLIENT_TYPE)
							{
								char buffer[256];
								sprintf(buffer, "qu%i", IDOfCharacter(gNetworkConnection->input->character));

								sendto(gNetworkConnection->socket, buffer, strlen(buffer), 0, (struct sockaddr *)&gNetworkConnection->hostAddress, sizeof(gNetworkConnection->hostAddress));
							}

							gNetworkConnection->shouldRun = SDL_FALSE;
						}
					}
					else /* if (gConsoleActivated) */
					{
						clearConsole();
					}
				}
			}

			else if (event->key.keysym.sym == SDLK_BACKQUOTE && gGameState)
			{
				if (gConsoleFlag)
				{
					if (gConsoleActivated)
					{
						gConsoleActivated = SDL_FALSE;
						SDL_EnableKeyRepeat(1, SDL_DEFAULT_REPEAT_INTERVAL);
					}
					else
					{
						gConsoleActivated = SDL_TRUE;
						SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);
					}
				}
			}

			else if (event->key.keysym.sym == SDLK_BACKSPACE)
			{
				if (gConsoleActivated)
				{
					performConsoleBackspace();
				}
				else if (gNetworkAddressFieldIsActive)
				{
					performNetworkAddressBackspace();
				}
				else if (gNetworkUserNameFieldIsActive)
				{
					performNetworkUserNameBackspace();
				}
			}

			// write text to console, make sure that none of the special keys are get in the text (shift, control, caps locks, etc)
			if (event->key.keysym.unicode != '`' && event->key.keysym.sym != SDLK_LCTRL
				&& event->key.keysym.sym != SDLK_RIGHT && event->key.keysym.sym != SDLK_LEFT
				&& event->key.keysym.sym != SDLK_UP && event->key.keysym.sym != SDLK_DOWN
				&& event->key.keysym.sym != SDLK_F1 && event->key.keysym.sym != SDLK_F2
				&& event->key.keysym.sym != SDLK_F3 && event->key.keysym.sym != SDLK_F4
				&& event->key.keysym.sym != SDLK_F4 && event->key.keysym.sym != SDLK_F5
				&& event->key.keysym.sym != SDLK_F6 && event->key.keysym.sym != SDLK_F7
				&& event->key.keysym.sym != SDLK_F8 && event->key.keysym.sym != SDLK_F9
				&& event->key.keysym.sym != SDLK_F10 && event->key.keysym.sym != SDLK_F11
				&& event->key.keysym.sym != SDLK_F12 && event->key.keysym.sym != SDLK_F13
				&& event->key.keysym.sym != SDLK_F14 && event->key.keysym.sym != SDLK_F15
				&& event->key.keysym.sym != SDLK_RCTRL && event->key.keysym.sym != SDLK_LALT
				&& event->key.keysym.sym != SDLK_RALT && event->key.keysym.sym != SDLK_LSHIFT
				&& event->key.keysym.sym != SDLK_RSHIFT && event->key.keysym.sym != SDLK_CAPSLOCK
				&& event->key.keysym.sym != SDLK_LMETA && event->key.keysym.sym != SDLK_RMETA
				&& event->key.keysym.sym != SDLK_RETURN && event->key.keysym.sym != SDLK_ESCAPE
				&& event->key.keysym.sym != SDLK_TAB && event->key.keysym.sym != SDLK_BACKSPACE
				&& event->key.keysym.sym != SDLK_UNKNOWN)
			{
				if (gConsoleActivated)
				{
					writeConsoleText((Uint8)event->key.keysym.unicode);
				}
				else if (gNetworkAddressFieldIsActive)
				{
					writeNetworkAddressText((Uint8)event->key.keysym.unicode);
				}
				else if (gNetworkUserNameFieldIsActive)
				{
					writeNetworkUserNameText((Uint8)event->key.keysym.unicode);
				}
			}

		case SDL_JOYBUTTONDOWN:
		case SDL_JOYAXISMOTION:
			if (gGameState && gGameWinner != NO_CHARACTER && (SDL_GetAppState() & SDL_APPINPUTFOCUS) &&
				(event->jbutton.button == gPinkBubbleGumInput.weapjs_id || event->jbutton.button == gRedRoverInput.weapjs_id ||
				event->jbutton.button == gBlueLightningInput.weapjs_id || event->jbutton.button == gGreenTreeInput.weapjs_id ||
				event->jaxis.axis == gPinkBubbleGumInput.weapjs_axis_id || event->jaxis.axis == gRedRoverInput.weapjs_id ||
				event->jaxis.axis == gBlueLightningInput.weapjs_axis_id || event->jaxis.axis == gGreenTreeInput.weapjs_axis_id))
			{
				// new game
				if (!gConsoleActivated && (!gNetworkConnection || gNetworkConnection->type != NETWORK_CLIENT_TYPE))
				{
					if (gNetworkConnection && gNetworkConnection->type == NETWORK_SERVER_TYPE)
					{
						sendToClients(0, "ng");
					}
					gGameShouldReset = SDL_TRUE;
				}
			}

			break;
		case SDL_FULLSCREEN_TOGGLE:
			// Reset video flags
			gVideoFlags = SDL_OPENGL;
			resizeWindow(gScreenWidth, gScreenHeight);

			if (gFullscreen)
			{
				createSurface(gVideoFlags);

				gFullscreen = SDL_FALSE;
				gFullscreenFlag = SDL_FALSE;
			}
			else
			{
				gVideoFlags |= SDL_FULLSCREEN;
				createSurface(gVideoFlags);

				gFullscreen = SDL_TRUE;
				gFullscreenFlag = SDL_TRUE;
			}

			// we need to fetch the current list of resolutions again, otherwise, SDL screws up the resolutions
			gResolutions = SDL_ListModes(NULL, SDL_FULLSCREEN | SDL_HWSURFACE);
			killResolutionsBelow(MIN_SCREEN_RESOLUTION_WIDTH, MIN_SCREEN_RESOLUTION_HEIGHT);
			
			break;
		case SDL_ACTIVEEVENT:
			if (!(SDL_GetAppState() & SDL_APPINPUTFOCUS))
			{
				pauseMusic();
			}
			else
			{
				unPauseMusic();
			}
			break;
		case SDL_QUIT:
			/* Save defaults */
			writeDefaults();

			if (gNetworkConnection)
			{
				if (gNetworkConnection->type == NETWORK_SERVER_TYPE)
				{
					sendToClients(0, "qu");
				}
				else if (gNetworkConnection->type == NETWORK_CLIENT_TYPE && gNetworkConnection->isConnected)
				{
					char buffer[256];
					sprintf(buffer, "qu%i", IDOfCharacter(gNetworkConnection->input->character));

					sendto(gNetworkConnection->socket, buffer, strlen(buffer), 0, (struct sockaddr *)&gNetworkConnection->hostAddress, sizeof(gNetworkConnection->hostAddress));
				}
			}

			*quit = SDL_TRUE;
			break;
	}

	/*
	 * Perform up and down key/joystick character actions when playing the game
	 * Make sure the user isn't trying to toggle fullscreen.
	 * Other actions, such as changing menus are dealt before here
	 */
	if (!((event->key.keysym.sym == SDLK_f || event->key.keysym.sym == SDLK_RETURN) && (SDL_GetModState() & KMOD_LMETA || SDL_GetModState() & KMOD_RMETA || SDL_GetModState() & KMOD_LALT || SDL_GetModState() & KMOD_RALT)) &&
		gGameState && (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP || event->type == SDL_JOYBUTTONDOWN || event->type == SDL_JOYBUTTONUP || event->type == SDL_JOYAXISMOTION))
	{
		if (!gConsoleActivated)
		{
			performDownAction(&gRedRoverInput, event);
			performDownAction(&gGreenTreeInput, event);
			performDownAction(&gPinkBubbleGumInput, event);
			performDownAction(&gBlueLightningInput, event);
		}

		performUpAction(&gRedRoverInput, event);
		performUpAction(&gGreenTreeInput, event);
		performUpAction(&gPinkBubbleGumInput, event);
		performUpAction(&gBlueLightningInput, event);
	}
}

static void resizeWindow(int width, int height)
{
	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);

    glLoadIdentity();

	gluPerspective(45.0f, width / height, 10.0f, 300.0f);

    glMatrixMode(GL_MODELVIEW);

	/*
	 * Resizing in SDL makes textures and display lists go bye-bye.
	 * Reload the textures and re-enable GL_DEPTH_TEST.
	 * If we don't re-enable it, the depth buffer will be screwed up
	 */
	glEnable(GL_DEPTH_TEST);
	
	deleteSceneryTextures();

	loadSceneryTextures();

	if (gGameLaunched)
	{
		deleteCharacterTextures();
		deleteCharacterLists();

		loadCharacterTextures();
		buildCharacterLists();

		deleteWeaponList(gRedRover.weap);
		deleteWeaponList(gGreenTree.weap);
		deleteWeaponList(gPinkBubbleGum.weap);
		deleteWeaponList(gBlueLightning.weap);

		buildWeaponList(gRedRover.weap);
		buildWeaponList(gGreenTree.weap);
		buildWeaponList(gPinkBubbleGum.weap);
		buildWeaponList(gBlueLightning.weap);
	}
	else
	{
		gGameLaunched = SDL_TRUE;
	}

	reloadGlyphs();
}

static void eventLoop(void)
{
	SDL_Event event;
	int done = 0;

	int fps = 30;
	int delay = 1000 / fps;
	int thenTicks = -1;
	int nowTicks;

	while (!done)
	{
		//check for events.
		while (SDL_PollEvent(&event))
		{
			eventInput(&event, &done);
		}

		drawScene();
		SDL_GL_SwapBuffers();
		
		SDL_bool hasAppFocus = (SDL_GetAppState() & SDL_APPINPUTFOCUS) != 0;
		// Restrict game to 30 fps when the fps flag is enabled as well as when we don't have app focus
		// This will allow the game to use less processing power when it's in the background,
		// which fixes a bug on macOS where the game can have huge CPU spikes when the window is completly obscured
		if (gFpsFlag || !hasAppFocus)
		{
			// time how long each draw-swap-delay cycle takes and adjust the delay to get closer to target framerate
			if (thenTicks > 0)
			{
				nowTicks = SDL_GetTicks();
				delay += (1000 / fps - (nowTicks - thenTicks));
				thenTicks = nowTicks;

				if (delay < 0)
					delay = 1000 / fps;
			}
			else
			{
				thenTicks = SDL_GetTicks();
			}

			SDL_Delay(delay);
		}

		// game has ended stuff should be here.
		if (gGameShouldReset)
		{
			endGame();
			initGame();
		}

		// deal with what music to play
		if (gAudioMusicFlag)
		{
			if (gGameState == GAME_STATE_OFF && !isPlayingMainMenuMusic())
			{
				stopMusic();
				if (hasAppFocus)
				{
					playMainMenuMusic();
				}
			}
			else if (gGameState == GAME_STATE_ON && !isPlayingGameMusic())
			{
				stopMusic();
				playGameMusic();
			}
		}
	}
}

/*
 * Returns a comparison result of rectOne and rectTwo resolutions.
 * 1 if rectOne > rectTwo
 * 2 if rectTwo > rectOne
 * 0 if rectOne == rectTwo
 */
static int compareResolutions(SDL_Rect rectOne, SDL_Rect rectTwo)
{
	if (rectOne.w > rectTwo.w)
		return 1;
	else if (rectOne.w < rectTwo.w)
		return 2;
	// the widths must be equal if we're here
	else if (rectOne.h > rectTwo.h)
		return 1;
	else if (rectOne.h < rectTwo.h)
		return 2;
	else
		return 0;
}

static void killResolutionsBelow(int width, int height)
{
	int resolutionCounter;
	SDL_Rect rectToKill;
	rectToKill.w = width;
	rectToKill.h = height;
	
	for (resolutionCounter = 0; gResolutions[resolutionCounter]; resolutionCounter++) 
	{
		if (compareResolutions(*gResolutions[resolutionCounter], rectToKill) == 2) 
		{
			gResolutions[resolutionCounter] = NULL; 
		}
	}
}

static SDL_bool findAndSetResolution(int width, int height)
{
	for (gResolutionCounter = 0; gResolutions[gResolutionCounter]; gResolutionCounter++)
	{
		if (gResolutions[gResolutionCounter]->w == width && gResolutions[gResolutionCounter]->h == height)
		{
			gScreenWidth = gResolutions[gResolutionCounter]->w;
			gScreenHeight = gResolutions[gResolutionCounter]->h;
			break;
		}
	}
	
	return (gScreenWidth != 0 && gScreenHeight != 0);
}

static void initSDL_GL(void)
{
	const SDL_VideoInfo *videoInfo;
	int bestResIndex;

	// Get video info
	videoInfo = SDL_GetVideoInfo();

	// The resolution that the user has on as default
	SDL_Rect bestResolution;

	bestResolution.w = videoInfo->current_w;
	bestResolution.h = videoInfo->current_h;

	gFullscreen = gFullscreenFlag;

	gResolutions = SDL_ListModes(NULL, SDL_FULLSCREEN | SDL_HWSURFACE);
	
	if (gResolutions == NULL)
	{
		zgPrint("gResolutions is NULL. Your display doesn't support any of the video resolutions");
		SDL_Terminate();
		return;
	}
	else  if (gResolutions == (SDL_Rect **)-1)
	{
		zgPrint("Your display supports all video gResolutions;^");
		zgPrint("however, you won't be able to choose which resolution you want in the video options menu.^");
		zgPrint("It will be 800x500 by default. To change this, you'll need to change the screen and height values in user_data.txt");
	}

	// if the defaults fail, try to use 800x500
	// if that fails, try to use 800x600,
	// if that fails, try to use 640x480
	if (gScreenWidth == 0 && gScreenHeight == 0)
	{
		if (!findAndSetResolution(800, 500) && !findAndSetResolution(800, 600) && !findAndSetResolution(MIN_SCREEN_RESOLUTION_WIDTH, MIN_SCREEN_RESOLUTION_HEIGHT))
		{
			zgPrint("Could not set default resolution!\n");
			exit(5);
		}
	}
	// otherwise find the users' screen resolution defaults
	else
	{
		// find gScreenWidth and gScreenHeight and set the counter accordingly
		for (gResolutionCounter = 0; gResolutions[gResolutionCounter]; gResolutionCounter++)
		{
			if (gResolutions[gResolutionCounter]->w == gScreenWidth && gResolutions[gResolutionCounter]->h == gScreenHeight)
				break;
		}

		if (compareResolutions(*(gResolutions[gResolutionCounter]), bestResolution) == 1)
		{
			zgPrint("Defaults resolution might be too high. Try trashing the defaults...Terminating...");
			SDL_Terminate();
			return;
		}
	}

	// Find the max resolution we can use for the video options. Store it in gBestResolutionLimit
	for (bestResIndex = 0; gResolutions[bestResIndex]; bestResIndex++)
	{
		if (compareResolutions(*gResolutions[bestResIndex], bestResolution) == 0)
		{
			gBestResolutionLimit = bestResIndex;
			break;
		}
	}
	
	killResolutionsBelow(MIN_SCREEN_RESOLUTION_WIDTH, MIN_SCREEN_RESOLUTION_HEIGHT);

	// Set up flags
	gVideoFlags = SDL_OPENGL;

	if (gFullscreen)
	{
		gVideoFlags |= SDL_FULLSCREEN;
	}

	// FSAA
	if (gFsaaFlag)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);
	}
	else
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
	}
	
	// VSYNC
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, gVsyncFlag);

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

#ifndef _DEBUG
	SDL_ShowCursor(SDL_DISABLE);
#endif

	createSurface(gVideoFlags);
}

static void createSurface(Uint32 flags)
{
	if (gScreen)
	{
		SDL_FreeSurface(gScreen);
	}

	gScreen = SDL_SetVideoMode(gScreenWidth, gScreenHeight, 0, flags);
	
	if (gScreen == NULL)
	{
		// setting the multi sample buffers and samples when the video device doesn't support it
		// may cause SDL_SetVideoMode to return NULL, so check if we can still create the screen without
		// the multi sample buffers and samples
		if (gFsaaFlag)
		{
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);

			gScreen = SDL_SetVideoMode(gScreenWidth, gScreenHeight, 0, flags);
		}

		if (gScreen == NULL)
		{
			zgPrint("Couldn't set %ix%i OpenGL video mode: %e", gScreenWidth, gScreenHeight);
			exit(4);
		}
	}
	
	resizeWindow(gScreenWidth, gScreenHeight);

#ifdef WINDOWS
	// Set the taskbar icon to the icon associated with our executable
	// For some reason, SDL messes this up
	struct SDL_SysWMinfo wmInfo;
	if (SDL_GetWMInfo(&wmInfo))
	{
		setWindowsIcon(wmInfo.window);
	}
#endif

	// The attributes we set initially may not be the same after we create the video mode, so
	// deal with it accordingly
	int value;
	SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &value);
	gFsaaFlag = value;

	SDL_GL_GetAttribute(SDL_GL_SWAP_CONTROL, &value);
	gVsyncFlag = value;
}

void initJoySticks(void)
{
	int numJoySticks = 0;
	int joyIndex;

	numJoySticks = SDL_NumJoysticks();

	if (numJoySticks > 0)
	{

		if (numJoySticks > 4)
		{
			zgPrint("There's more than 4 joysticks available. We're going to only read the first four joysticks connected to the system.");
			numJoySticks = 4;
		}

		for (joyIndex = 0; joyIndex < numJoySticks; joyIndex++)
		{
			SDL_JoystickOpen(joyIndex);
		}
	}
}

int main(int argc, char *argv[])
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO) < 0)
	{
        zgPrint("Couldn't initialize SDL: %e");
		return -1;
	}

	// On Mac OS X we use the system GLUT which is already initialized
#ifndef MAC_OS_X
	glutInit(&argc, argv);
#endif
	
#ifdef WINDOWS
	SDL_WM_SetCaption("", NULL);
#endif

	initJoySticks();

	if (!initFont())
	{
		return -2;
	}

	if (!initAudio())
	{
		return -3;
	};

	SDL_EnableUNICODE(SDL_TRUE);

	readDefaults();

	initSDL_GL();
    initGL();

    eventLoop();

	SDL_Quit();

    return 0;
}
