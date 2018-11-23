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

#define MATH_3D_IMPLEMENTATION
#include "math_3d.h"

static const int GAME_STATE_OFF =				0;
static const int GAME_STATE_ON =				1;

// in seconds
static const int NEW_GAME_WILL_BEGIN_DELAY =	3;

SDL_bool gGameHasStarted;
SDL_bool gGameShouldReset;
int gGameStartNumber;
int gGameWinner;

// Console flag indicating if we can use the console
SDL_bool gConsoleFlag =							SDL_FALSE;

// audio flags
SDL_bool gAudioEffectsFlag =					SDL_TRUE;
SDL_bool gAudioMusicFlag =						SDL_TRUE;

// video flags
SDL_bool gFpsFlag =								SDL_FALSE;

SDL_bool gValidDefaults =						SDL_FALSE;

// Lives
int gCharacterLives =							5;
int gCharacterNetLives =						5;

// Fullscreen video state.
static SDL_bool gConsoleActivated =				SDL_FALSE;
SDL_bool gDrawFPS =								SDL_FALSE;

static int gGameState;

#define MAX_CHARACTER_LIVES 10

void initGame(SDL_Window *window);

static void initScene(Renderer *renderer);

static void readDefaults(void);
static void writeDefaults(Renderer *renderer);

static void drawBlackBox(Renderer *renderer)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){0.0f, 0.0f, -25.0f});
	
	float vertices[] =
	{
		-17.3f, 17.3f, -13.0f,
		17.3f, 17.3f, -13.0f,
		17.3f, -17.3f, -13.0f,
		-17.3f, -17.3f, -13.0f
	};
	
	uint8_t indices[] =
	{
		0, 1, 2,
		2, 3, 0
	};
	
	drawVerticesFromIndices(renderer, modelViewMatrix, RENDERER_TRIANGLE_MODE, vertices, 3, indices, RENDERER_INT8_TYPE, sizeof(indices) / sizeof(*indices), (color4_t){0.0f, 0.0f, 0.0f, 0.7f}, RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA | RENDERER_OPTION_DISABLE_DEPTH_TEST);
}

static void initScene(Renderer *renderer)
{
	loadSceneryTextures(renderer);

	initTiles();

	initCharacters();

	loadCharacterTextures(renderer);
	buildCharacterModels();

	// defaults couldn't be read
	if (!gValidDefaults)
	{
		// init the inputs
		initInput(&gRedRoverInput, SDL_SCANCODE_B, SDL_SCANCODE_C, SDL_SCANCODE_F, SDL_SCANCODE_V, SDL_SCANCODE_G);
		initInput(&gGreenTreeInput, SDL_SCANCODE_L, SDL_SCANCODE_J, SDL_SCANCODE_I, SDL_SCANCODE_K, SDL_SCANCODE_M);
		initInput(&gBlueLightningInput, SDL_SCANCODE_D, SDL_SCANCODE_A, SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_Z);
		initInput(&gPinkBubbleGumInput, SDL_SCANCODE_RIGHT, SDL_SCANCODE_LEFT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_SPACE);

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

#define MAX_EXPECTED_SCAN_LENGTH 512
static SDL_bool scanExpectedString(FILE *fp, const char *expectedString)
{
	size_t expectedStringLength = strlen(expectedString);
	if (expectedStringLength > MAX_EXPECTED_SCAN_LENGTH - 1)
	{
		return SDL_FALSE;
	}
	
	char buffer[MAX_EXPECTED_SCAN_LENGTH] = {0};
	if (fread(buffer, expectedStringLength, 1, fp) < 1)
	{
		return SDL_FALSE;
	}
	
	return (strcmp(buffer, expectedString) == 0);
}

static SDL_bool scanLineTerminatingString(FILE *fp, char *destBuffer, size_t maxBufferSize)
{
	char *safeBuffer = calloc(maxBufferSize, 1);
	if (safeBuffer == NULL) return SDL_FALSE;
	
	size_t safeBufferIndex = 0;
	while (safeBufferIndex < maxBufferSize - 1 && ferror(fp) == 0)
	{
		int byte = fgetc(fp);
		if (byte == EOF || byte == '\n') break;
		
		safeBuffer[safeBufferIndex++] = (char)byte;
	}
	
	SDL_bool success;
	if (ferror(fp) != 0 || feof(fp) != 0 || safeBufferIndex >= maxBufferSize - 1)
	{
		success = SDL_FALSE;
	}
	else
	{
		memcpy(destBuffer, safeBuffer, maxBufferSize);
		success = SDL_TRUE;
	}
	
	free(safeBuffer);
	
	return success;
}

static SDL_bool scanGroupedJoyString(FILE *fp, char *joyBuffer)
{
	char tempBuffer[MAX_JOY_DESCRIPTION_BUFFER_LENGTH + 2] = {0};
	if (!scanLineTerminatingString(fp, tempBuffer, sizeof(tempBuffer))) return SDL_FALSE;
	
	size_t length = strlen(tempBuffer);
	if (length <= 2) return SDL_FALSE;
	
	if (tempBuffer[0] != '(' || tempBuffer[length - 1] != ')') return SDL_FALSE;
	
	memcpy(joyBuffer, tempBuffer + 1, length - 2);
	
	return SDL_TRUE;
}

static SDL_bool readCharacterInputDefaults(FILE *fp, const char *characterName, Input *input)
{
	if (!scanExpectedString(fp, characterName)) return SDL_FALSE;
	if (fscanf(fp, " key right: %i\n", &input->r_id) < 1) return SDL_FALSE;
	
	if (!scanExpectedString(fp, characterName)) return SDL_FALSE;
	if (fscanf(fp, " key left: %i\n", &input->l_id) < 1) return SDL_FALSE;
	
	if (!scanExpectedString(fp, characterName)) return SDL_FALSE;
	if (fscanf(fp, " key up: %i\n", &input->u_id) < 1) return SDL_FALSE;
	
	if (!scanExpectedString(fp, characterName)) return SDL_FALSE;
	if (fscanf(fp, " key down: %i\n", &input->d_id) < 1) return SDL_FALSE;
	
	if (!scanExpectedString(fp, characterName)) return SDL_FALSE;
	if (fscanf(fp, " key weapon: %i\n", &input->weap_id) < 1) return SDL_FALSE;
	
	input->joy_right = calloc(MAX_JOY_DESCRIPTION_BUFFER_LENGTH, 1);
	input->joy_left = calloc(MAX_JOY_DESCRIPTION_BUFFER_LENGTH, 1);
	input->joy_up = calloc(MAX_JOY_DESCRIPTION_BUFFER_LENGTH, 1);
	input->joy_down = calloc(MAX_JOY_DESCRIPTION_BUFFER_LENGTH, 1);
	input->joy_weap = calloc(MAX_JOY_DESCRIPTION_BUFFER_LENGTH, 1);
	
	if (!scanExpectedString(fp, characterName)) return SDL_FALSE;
	if (fscanf(fp, " joy right id: %i, axis: %i, joy id: %i ", &input->rjs_id, &input->rjs_axis_id, &input->joy_right_id) < 3) return SDL_FALSE;
	if (!scanGroupedJoyString(fp, input->joy_right)) return SDL_FALSE;
	
	if (!scanExpectedString(fp, characterName)) return SDL_FALSE;
	if (fscanf(fp, " joy left id: %i, axis: %i, joy id: %i ", &input->ljs_id, &input->ljs_axis_id, &input->joy_left_id) < 3) return SDL_FALSE;
	if (!scanGroupedJoyString(fp, input->joy_left)) return SDL_FALSE;
	
	if (!scanExpectedString(fp, characterName)) return SDL_FALSE;
	if (fscanf(fp, " joy up id: %i, axis: %i, joy id: %i ", &input->ujs_id, &input->ujs_axis_id, &input->joy_up_id) < 3) return SDL_FALSE;
	if (!scanGroupedJoyString(fp, input->joy_up)) return SDL_FALSE;
	
	if (!scanExpectedString(fp, characterName)) return SDL_FALSE;
	if (fscanf(fp, " joy down id: %i, axis: %i, joy id: %i ", &input->djs_id, &input->djs_axis_id, &input->joy_down_id) < 3) return SDL_FALSE;
	if (!scanGroupedJoyString(fp, input->joy_down)) return SDL_FALSE;
	
	if (!scanExpectedString(fp, characterName)) return SDL_FALSE;
	if (fscanf(fp, " joy weapon id: %i, axis: %i, joy id: %i ", &input->weapjs_id, &input->weapjs_axis_id, &input->joy_weap_id) < 3) return SDL_FALSE;
	if (!scanGroupedJoyString(fp, input->joy_weap)) return SDL_FALSE;
	
	if (!scanExpectedString(fp, "\n")) return SDL_FALSE;
	
	return SDL_TRUE;
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

	// conole flag default
	int consoleFlag = 0;
	if (fscanf(fp, "Console flag: %i\n\n", &consoleFlag) < 1) goto cleanup;
	gConsoleFlag = (consoleFlag != 0);

	// video defaults
	int screenWidth = 0;
	if (fscanf(fp, "screen width: %i\n", &screenWidth) < 1) goto cleanup;
	
	int screenHeight = 0;
	if (fscanf(fp, "screen height: %i\n", &screenHeight) < 1) goto cleanup;
	
	int vsyncFlag = 0;
	if (fscanf(fp, "vsync flag: %i\n", &vsyncFlag) < 1) goto cleanup;
	
	int fpsFlag = 0;
	if (fscanf(fp, "fps flag: %i\n", &fpsFlag) < 1) goto cleanup;
	
	int aaFlag = 0;
	if (fscanf(fp, "FSAA flag: %i\n", &aaFlag) < 1) goto cleanup;

	int fullscreenFlag = 0;
	if (fscanf(fp, "Fullscreen flag: %i\n", &fullscreenFlag) < 1) goto cleanup;

	if (fscanf(fp, "Number of lives: %i\n", &gCharacterLives) < 1) goto cleanup;
	if (gCharacterLives > MAX_CHARACTER_LIVES)
	{
		gCharacterLives = MAX_CHARACTER_LIVES;
	}
	else if (gCharacterLives < 0)
	{
		gCharacterLives = MAX_CHARACTER_LIVES / 2;
	}
	
	if (fscanf(fp, "Number of net lives: %i\n", &gCharacterNetLives) < 1) goto cleanup;
	if (gCharacterNetLives > MAX_CHARACTER_LIVES)
	{
		gCharacterNetLives = MAX_CHARACTER_LIVES;
	}
	else if (gCharacterNetLives < 0)
	{
		gCharacterNetLives = MAX_CHARACTER_LIVES / 2;
	}

	// character states
	if (fscanf(fp, "Pink Bubblegum state: %i\n", &gPinkBubbleGum.state) < 1) goto cleanup;
	if (gPinkBubbleGum.state != CHARACTER_HUMAN_STATE && gPinkBubbleGum.state != CHARACTER_AI_STATE)
	{
		gPinkBubbleGum.state = CHARACTER_HUMAN_STATE;
	}
	
	if (fscanf(fp, "Red Rover state: %i\n", &gRedRover.state) < 1) goto cleanup;
	if (gRedRover.state != CHARACTER_HUMAN_STATE && gRedRover.state != CHARACTER_AI_STATE)
	{
		gRedRover.state = CHARACTER_AI_STATE;
	}
	
	if (fscanf(fp, "Green Tree state: %i\n", &gGreenTree.state) < 1) goto cleanup;
	if (gGreenTree.state != CHARACTER_HUMAN_STATE && gGreenTree.state != CHARACTER_AI_STATE)
	{
		gGreenTree.state = CHARACTER_AI_STATE;
	}
	
	if (fscanf(fp, "Blue Lightning state: %i\n", &gBlueLightning.state) < 1) goto cleanup;
	if (gBlueLightning.state != CHARACTER_HUMAN_STATE && gBlueLightning.state != CHARACTER_AI_STATE)
	{
		gBlueLightning.state = CHARACTER_AI_STATE;
	}

	if (fscanf(fp, "AI Mode: %i\n", &gAIMode) < 1) goto cleanup;
	if (gAIMode != AI_EASY_MODE && gAIMode != AI_MEDIUM_MODE && gAIMode != AI_HARD_MODE)
	{
		gAIMode = AI_EASY_MODE;
	}
	
	if (fscanf(fp, "AI Net Mode: %i\n", &gAINetMode) < 1) goto cleanup;
	if (gAINetMode != AI_EASY_MODE && gAINetMode != AI_MEDIUM_MODE && gAINetMode != AI_HARD_MODE)
	{
		gAINetMode = AI_EASY_MODE;
	}
	
	if (fscanf(fp, "Number of Net Humans: %i\n", &gNumberOfNetHumans) < 1) goto cleanup;
	if (gNumberOfNetHumans < 0 || gNumberOfNetHumans > 3)
	{
		gNumberOfNetHumans = 1;
	}
	
	if (!readCharacterInputDefaults(fp, "Pink Bubblegum", &gPinkBubbleGumInput)) goto cleanup;
	if (!readCharacterInputDefaults(fp, "Red Rover", &gRedRoverInput)) goto cleanup;
	if (!readCharacterInputDefaults(fp, "Green Tree", &gGreenTreeInput)) goto cleanup;
	if (!readCharacterInputDefaults(fp, "Blue Lightning", &gBlueLightningInput)) goto cleanup;
	
	if (!scanExpectedString(fp, "Server IP Address: ")) goto cleanup;
	if (!scanLineTerminatingString(fp, gServerAddressString, sizeof(gServerAddressString))) goto cleanup;
	
	gServerAddressStringIndex = strlen(gServerAddressString);
	
	if (!scanExpectedString(fp, "\nNet name: ")) goto cleanup;
	if (!scanLineTerminatingString(fp, gUserNameString, sizeof(gUserNameString))) goto cleanup;
	
	gUserNameStringIndex = strlen(gUserNameString);
	
	int audioEffectsFlag = 0;
	if (fscanf(fp, "\nAudio effects: %i\n", &audioEffectsFlag) < 1) goto cleanup;
	gAudioEffectsFlag = (audioEffectsFlag != 0);
	
	int audioMusicFlag = 0;
	if (fscanf(fp, "Music: %i\n", &audioMusicFlag) < 1) goto cleanup;
	gAudioMusicFlag = (audioMusicFlag != 0);

	gValidDefaults = SDL_TRUE;
cleanup:
	fclose(fp);
}

static void writeDefaults(Renderer *renderer)
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
	fprintf(fp, "screen width: %i\n", renderer->windowWidth);
	fprintf(fp, "screen height: %i\n", renderer->windowHeight);
	fprintf(fp, "vsync flag: %i\n", renderer->vsync);
	fprintf(fp, "fps flag: %i\n", gFpsFlag);
	fprintf(fp, "FSAA flag: %i\n", renderer->fsaa);
	fprintf(fp, "Fullscreen flag: %i\n", 1);

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

void initGame(SDL_Window *window)
{
	loadTiles();

	loadCharacter(&gRedRover);
	loadCharacter(&gGreenTree);
	loadCharacter(&gPinkBubbleGum);
	loadCharacter(&gBlueLightning);

	if (!startAnimation(window))
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
}

void endGame(void)
{
	gGameHasStarted = SDL_FALSE;

	endAnimation();

	gGameState = GAME_STATE_OFF;

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
void drawFramesPerSecond(Renderer *renderer)
{
	static unsigned frame_count = 0;
    static double last_fps_time = -1.0;
    static double last_frame_time = -1.0;
	double now;
	double dt_fps;

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
		sprintf(fpsString, "%d", (int)(frame_count / dt_fps));

        frame_count = 0;
        last_fps_time = now;
    }

	frame_count++;

	// Draw the string
	size_t length = strlen(fpsString);
	if (length > 0)
	{
		mat4_t modelViewMatrix = m4_translation((vec3_t){9.0f, 9.0f, -25.0f});
		drawString(renderer, modelViewMatrix, (color4_t){0.0f, 0.5f, 0.8f, 1.0f}, 0.16f * length, 0.5f, fpsString);
	}
}

static void drawScoresForCharacter(Renderer *renderer, Character *character, color4_t color, float x, float y, float z)
{
	mat4_t iconModelViewMatrix = m4_translation((vec3_t){x, y, z});
	drawCharacterIcon(renderer, iconModelViewMatrix, character);
	
	color4_t characterColor = (color4_t){character->red, character->green, character->blue, 0.7f};
	
	mat4_t winsLabelModelViewMatrix = m4_mul(iconModelViewMatrix, m4_translation((vec3_t){0.0f, -2.0f, 0.0f}));
	drawString(renderer, winsLabelModelViewMatrix, characterColor, 0.5f, 0.5f, "Wins:");
	
	mat4_t winsModelViewMatrix = m4_mul(winsLabelModelViewMatrix, m4_translation((vec3_t){1.0f, 0.0f, 0.0f}));
	drawStringf(renderer, winsModelViewMatrix, characterColor, 0.5f, 0.5f, "%i", character->wins);
	
	mat4_t killsLabelModelViewMatrix = m4_mul(winsModelViewMatrix, m4_translation((vec3_t){-1.0f, -2.0f, 0.0f}));
	drawString(renderer, killsLabelModelViewMatrix, characterColor, 0.5f, 0.5f, "Kills:");
	
	mat4_t killsModelViewMatrix = m4_mul(killsLabelModelViewMatrix, m4_translation((vec3_t){1.0f, 0.0f, 0.0f}));
	drawStringf(renderer, killsModelViewMatrix, characterColor, 0.5f, 0.5f, "%i", character->kills);
}

static void drawScene(Renderer *renderer)
{
	clearColorAndDepthBuffers(renderer);

	if (gGameState)
	{
		drawWeapon(renderer, gRedRover.weap);
		drawWeapon(renderer, gGreenTree.weap);
		drawWeapon(renderer, gPinkBubbleGum.weap);
		drawWeapon(renderer, gBlueLightning.weap);

		drawCharacter(renderer, &gRedRover);
		drawCharacter(renderer, &gGreenTree);
		drawCharacter(renderer, &gPinkBubbleGum);
		drawCharacter(renderer, &gBlueLightning);

		drawTiles(renderer);

		if (gDrawFPS)
		{
			drawFramesPerSecond(renderer);
		}

		if (!gGameHasStarted)
		{
			color4_t textColor = (color4_t){0.0, 0.0, 1.0, 0.5};
			
			mat4_t modelViewMatrix = m4_translation((vec3_t){-1.0, 80.0, -280.0});

			if (gPinkBubbleGum.netState == NETWORK_PENDING_STATE || gRedRover.netState == NETWORK_PENDING_STATE || gGreenTree.netState == NETWORK_PENDING_STATE || gBlueLightning.netState == NETWORK_PENDING_STATE)
			{
				if (gNetworkConnection)
				{
					// be sure to take account the plural form of player(s)
					if (gNetworkConnection->numberOfPlayersToWaitFor > 1)
					{
						drawStringf(renderer, modelViewMatrix, textColor, 50.0, 10.0, "Waiting for %i players to connect...", gNetworkConnection->numberOfPlayersToWaitFor);
					}
					else if (gNetworkConnection->numberOfPlayersToWaitFor == 0)
					{
						drawString(renderer, modelViewMatrix, textColor, 50.0, 10.0, "Waiting for players to connect...");
					}
					else
					{
						drawString(renderer, modelViewMatrix, textColor, 50.0, 10.0, "Waiting for 1 player to connect...");
					}
				}
			}

			else if (gGameStartNumber > 0)
			{
				drawStringf(renderer, modelViewMatrix, textColor, 40.0, 10.0, "Game begins in %i", gGameStartNumber);
			}

			else if (gGameStartNumber == 0)
			{
				gGameHasStarted = SDL_TRUE;
			}
		}
	}

	// For this blending to work properly, we have to draw the sky *right* here.
	drawSky(renderer);

	if (gGameState)
	{
		drawCharacterIcons(renderer);

		drawCharacterLives(renderer);

		if (gConsoleActivated)
		{
			drawConsole(renderer);
			drawConsoleText(renderer);
		}

		if (gGameWinner != NO_CHARACTER)
		{
			mat4_t winLoseModelViewMatrix = m4_translation((vec3_t){70.0f, 100.0f, -280.0f});
			color4_t textColor = (color4_t){1.0f, 1.0f, 1.0f, 0.8f};

			if (gNetworkConnection)
			{
				if (gGameWinner == IDOfCharacter(gNetworkConnection->input->character))
				{
					drawString(renderer, winLoseModelViewMatrix, textColor, 20.0, 10.0, "You win!");
				}
				else
				{
					drawString(renderer, winLoseModelViewMatrix, textColor, 20.0, 10.0, "You lose!");
				}
			}
			else
			{
				if (gGameWinner == RED_ROVER)
				{
					drawString(renderer, winLoseModelViewMatrix, textColor, 40.0, 10.0, "Red Rover wins!");
				}
				else if (gGameWinner == GREEN_TREE)
				{
					drawString(renderer, winLoseModelViewMatrix, textColor, 40.0, 10.0, "Green Tree wins!");
				}
				else if (gGameWinner == PINK_BUBBLE_GUM)
				{
					drawString(renderer, winLoseModelViewMatrix, textColor, 40.0, 10.0, "Pink Bubblegum wins!");
				}
				else if (gGameWinner == BLUE_LIGHTNING)
				{
					drawString(renderer, winLoseModelViewMatrix, textColor, 40.0, 10.0, "Blue Lightning wins!");
				}
			}

			/* Stats */
			
			drawBlackBox(renderer);

			/* Display stats */
			
			drawScoresForCharacter(renderer, &gPinkBubbleGum, textColor, -6.0, 7.0, -25.0);
			drawScoresForCharacter(renderer, &gRedRover, textColor, -2.0f, 7.0f, -25.0f);
			drawScoresForCharacter(renderer, &gGreenTree, textColor, 2.0f, 7.0f, -25.0f);
			drawScoresForCharacter(renderer, &gBlueLightning, textColor, 6.0f, 7.0f, -25.0f);

			if (!gNetworkConnection || gNetworkConnection->type != NETWORK_CLIENT_TYPE)
			{
				// Draw a "Press ENTER to play again" notice
				mat4_t modelViewMatrix = m4_translation((vec3_t){0.0f, -7.0f, -25.0f});

				drawString(renderer, modelViewMatrix, (color4_t){0.0f, 0.0f, 0.4f, 0.4f}, 5.0f, 1.0f, "Fire to play again or Escape to quit");
			}
		}
	}

	if (!gGameState)
	{
		drawBlackBox(renderer);
		
		mat4_t gameTitleModelViewMatrix = m4_translation((vec3_t){-1.0f, 27.0f, -100.0f});
		
		drawString(renderer, gameTitleModelViewMatrix, (color4_t){0.3f, 0.2f, 1.0f, 0.35f}, 20.0, 5.0, "Sky Checkers");

		drawMenus(renderer);

		if (isChildBeingDrawn(&gJoyStickConfig[0][1]) /* pinkBubbleGumConfigRightJoyStick */	||
			isChildBeingDrawn(&gJoyStickConfig[1][1]) /* redRoverConfigRightJoyStick */			||
			isChildBeingDrawn(&gJoyStickConfig[2][1]) /* greenTreeRightgJoyStickConfig */		||
			isChildBeingDrawn(&gJoyStickConfig[3][1]) /* blueLightningConfigJoyStick */)
		{
			mat4_t translationMatrix = m4_translation((vec3_t){-1.0f, 50.0f, -280.0f});
			mat4_t rotationMatrix = m4_rotation_x(M_PI / 4.0f);
			mat4_t instructionsModelViewMatrix = m4_mul(translationMatrix, rotationMatrix);
			
			color4_t textColor = (color4_t){0.3f, 0.2f, 1.0f, 0.6f};

			drawString(renderer, instructionsModelViewMatrix, textColor, 100.0, 5.0, "Click enter to modify a mapping value and input in a button on your joystick. Click Escape to exit out.");
			
			mat4_t noticeModelViewMatrix = m4_mul(instructionsModelViewMatrix, m4_translation((vec3_t){0.0f, -20.0f, 0.0f}));

			drawString(renderer, noticeModelViewMatrix, textColor, 50.0, 5.0, "(Joysticks only function in-game)");
		}

		else if (isChildBeingDrawn(&gCharacterConfigureKeys[0][1]) /* pinkBubbleGum */	||
				 isChildBeingDrawn(&gCharacterConfigureKeys[1][1]) /* redRover */		||
				 isChildBeingDrawn(&gCharacterConfigureKeys[2][1]) /* greenTree */		||
				 isChildBeingDrawn(&gCharacterConfigureKeys[3][1]) /* blueLightning */)
		{
			mat4_t translationMatrix = m4_translation((vec3_t){-1.0f, 50.0f, -280.0f});
			mat4_t rotationMatrix = m4_rotation_x(M_PI / 4.0f);
			mat4_t instructionsModelViewMatrix = m4_mul(translationMatrix, rotationMatrix);
			
			color4_t textColor = (color4_t){0.3f, 0.2f, 1.0f, 0.6f};

			drawString(renderer, instructionsModelViewMatrix, textColor, 100.0, 5.0, "Click enter to modify a mapping value and input in a key. Click Escape to exit out.");
		}
	}
}

static void eventInput(SDL_Event *event, SDL_Window *window, int *quit)
{
	switch (event->type)
	{
		case SDL_KEYDOWN:
			if (!gConsoleActivated && gGameState && gGameWinner != NO_CHARACTER &&
				(event->key.keysym.scancode == gPinkBubbleGumInput.weap_id || event->key.keysym.scancode == gRedRoverInput.weap_id ||
				 event->key.keysym.scancode == gBlueLightningInput.weap_id || event->key.keysym.scancode == gGreenTreeInput.weap_id ||
				event->key.keysym.scancode == SDL_SCANCODE_RETURN || event->key.keysym.scancode == SDL_SCANCODE_KP_ENTER))
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

			if (event->key.keysym.scancode == SDL_SCANCODE_RETURN || event->key.keysym.scancode == SDL_SCANCODE_KP_ENTER)
			{
				if (gCurrentMenu == gConfigureLivesMenu)
				{
					gDrawArrowsForCharacterLivesFlag = !gDrawArrowsForCharacterLivesFlag;
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
					invokeMenu(window);
				}

				else if (gConsoleActivated)
				{
					executeConsoleCommand();
				}
			}

			else if (event->key.keysym.scancode == SDL_SCANCODE_DOWN)
			{
				if (gNetworkAddressFieldIsActive)
					break;

				if (gDrawArrowsForCharacterLivesFlag)
				{
					if (gCharacterLives == 1)
						gCharacterLives = MAX_CHARACTER_LIVES;
					else
						gCharacterLives--;
				}
				else if (gDrawArrowsForNetPlayerLivesFlag)
				{
					if (gCharacterNetLives == 1)
					{
						gCharacterNetLives = MAX_CHARACTER_LIVES;
					}
					else
					{
						gCharacterNetLives--;
					}
					
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

			else if (event->key.keysym.scancode == SDL_SCANCODE_UP)
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

			else if (event->key.keysym.scancode == SDL_SCANCODE_ESCAPE)
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
					else if (gDrawArrowsForAIModeFlag)
					{
						gDrawArrowsForAIModeFlag = SDL_FALSE;
					}
					else if (gDrawArrowsForNumberOfNetHumansFlag)
					{
						gDrawArrowsForNumberOfNetHumansFlag = SDL_FALSE;
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
					}
					else
					{
						gConsoleActivated = SDL_TRUE;
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
		case SDL_TEXTINPUT:
			if (gConsoleActivated || gNetworkAddressFieldIsActive || gNetworkUserNameFieldIsActive)
			{
				char *text = event->text.text;
				for (uint8_t textIndex = 0; textIndex < SDL_TEXTINPUTEVENT_TEXT_SIZE; textIndex++)
				{
					if (text[textIndex] == 0x0 || text[textIndex] == 0x1 || text[textIndex] == '`' || text[textIndex] == '~')
					{
						break;
					}
					
					if (gConsoleActivated)
					{
						writeConsoleText((Uint8)text[textIndex]);
					}
					else if (gNetworkAddressFieldIsActive)
					{
						writeNetworkAddressText((Uint8)text[textIndex]);
					}
					else if (gNetworkUserNameFieldIsActive)
					{
						writeNetworkUserNameText((Uint8)text[textIndex]);
					}
				}
			}
			
			break;

		case SDL_JOYBUTTONDOWN:
		case SDL_JOYAXISMOTION:
			if (gGameState && gGameWinner != NO_CHARACTER && ((SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0) &&
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
		case SDL_WINDOWEVENT:
			if (event->window.event == SDL_WINDOWEVENT_FOCUS_LOST)
			{
				pauseMusic();
			}
			else if (event->window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
			{
				unPauseMusic();
			}
			break;
		case SDL_QUIT:
			*quit = SDL_TRUE;
			break;
	}

	/*
	 * Perform up and down key/joystick character actions when playing the game
	 * Make sure the user isn't trying to toggle fullscreen.
	 * Other actions, such as changing menus are dealt before here
	 */
	if (!((event->key.keysym.sym == SDLK_f || event->key.keysym.sym == SDLK_RETURN) && (((SDL_GetModState() & KMOD_LGUI) != 0) || ((SDL_GetModState() & KMOD_RGUI) != 0) || ((SDL_GetModState() & KMOD_LALT) != 0) || ((SDL_GetModState() & KMOD_RALT) != 0))) &&
		gGameState && (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP || event->type == SDL_JOYBUTTONDOWN || event->type == SDL_JOYBUTTONUP || event->type == SDL_JOYAXISMOTION))
	{
		if (!gConsoleActivated)
		{
			performDownAction(&gRedRoverInput, window, event);
			performDownAction(&gGreenTreeInput, window, event);
			performDownAction(&gPinkBubbleGumInput, window, event);
			performDownAction(&gBlueLightningInput, window, event);
		}

		performUpAction(&gRedRoverInput, window, event);
		performUpAction(&gGreenTreeInput, window, event);
		performUpAction(&gPinkBubbleGumInput, window, event);
		performUpAction(&gBlueLightningInput, window, event);
	}
}

static void eventLoop(Renderer *renderer)
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
			eventInput(&event, renderer->window, &done);
		}

		drawScene(renderer);
		swapBuffers(renderer);
		
		SDL_bool hasAppFocus = (SDL_GetWindowFlags(renderer->window) & SDL_WINDOW_INPUT_FOCUS) != 0;
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
			initGame(renderer->window);
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

	initJoySticks();

	if (!initFont())
	{
		return -2;
	}

	if (!initAudio())
	{
		return -3;
	};

	readDefaults();

	// Create renderer
	uint32_t videoFlags = SDL_WINDOW_OPENGL;
#ifndef _DEBUG
	videoFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
#endif
	
	int32_t windowWidth;
	int32_t windowHeight;
#ifndef _DEBUG
	windowWidth = 0;
	windowHeight = 0;
#else
	windowWidth = 800;
	windowHeight = 500;
#endif
	
	SDL_bool vsync = SDL_TRUE;
	SDL_bool fsaa = SDL_TRUE;
	
	Renderer renderer;
	createRenderer(&renderer, windowWidth, windowHeight, videoFlags, vsync, fsaa);
	
	// Initialize game related things
	// init random number generator
	mt_init();
	
	initScene(&renderer);
	
	/*
	 * Load a few font strings before a game starts up.
	 * We don't want the user to experience slow font loading times during a game
	 */
	cacheString(&renderer, "Game begins in 1");
	cacheString(&renderer, "Game begins in 2");
	cacheString(&renderer, "Game begins in 3");
	cacheString(&renderer, "Game begins in 4");
	cacheString(&renderer, "Game begins in 5");
	
	cacheString(&renderer, "Wins:");
	cacheString(&renderer, "Kills:");

	// Start the game event loop
    eventLoop(&renderer);
	
	// Prepare to quit
	
	// Save user defaults
	writeDefaults(&renderer);
	
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

	SDL_Quit();
    return 0;
}
