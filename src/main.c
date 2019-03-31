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

// in seconds
static const int NEW_GAME_WILL_BEGIN_DELAY =	3;

SDL_bool gGameHasStarted;
SDL_bool gGameShouldReset;
int32_t gGameStartNumber;
int gGameWinner;

// Console flag indicating if we can use the console
SDL_bool gConsoleFlag = SDL_FALSE;

// audio flags
SDL_bool gAudioEffectsFlag = SDL_TRUE;
SDL_bool gAudioMusicFlag = SDL_TRUE;

// video flags
static SDL_bool gFpsFlag = SDL_FALSE;
static SDL_bool gFullscreenFlag = SDL_FALSE;
static SDL_bool gVsyncFlag = SDL_TRUE;
static SDL_bool gFsaaFlag = SDL_TRUE;
static int32_t gWindowWidth = 800;
static int32_t gWindowHeight = 500;

SDL_bool gValidDefaults = SDL_FALSE;

// Lives
int gCharacterLives =							5;
int gCharacterNetLives =						5;

// Fullscreen video state.
static SDL_bool gConsoleActivated =				SDL_FALSE;
SDL_bool gDrawFPS =								SDL_FALSE;
SDL_bool gDrawPings = SDL_FALSE;

static GameState gGameState;

static uint32_t gEscapeHeldDownTimer;

#define MAX_CHARACTER_LIVES 10
#define CHARACTER_ICON_DISPLACEMENT 5.0f
#define CHARACTER_ICON_OFFSET -8.5f

#define MAX_SUPPORTED_JOYSTICKS 32

void initGame(void);

static void initScene(Renderer *renderer);

static void readDefaults(SDL_Joystick **joysticks);
static void writeDefaults(Renderer *renderer);

static void removeJoyStick(SDL_Joystick **joysticks, SDL_JoystickID instanceID);
static void addJoyStick(SDL_Joystick **joysticks, int deviceIndex);
static void setJoystickIdFromGuid(SDL_Joystick **joysticks, int *joystickId, char *guidString);
static void reloadJoySticks(SDL_Joystick **joysticks);

static void drawBlackBox(Renderer *renderer)
{
	static BufferArrayObject vertexArrayObject;
	static BufferObject indicesBufferObject;
	static SDL_bool initializedBuffers;
	
	if (!initializedBuffers)
	{
		const float vertices[] =
		{
			-9.5f, 9.5f, 0.0f,
			9.5f, 9.5f, 0.0f,
			9.5f, -9.5f, 0.0f,
			-9.5f, -9.5f, 0.0f
		};
		
		const uint16_t indices[] =
		{
			0, 1, 2,
			2, 3, 0
		};
		
		vertexArrayObject = createVertexArrayObject(renderer, vertices, sizeof(vertices));
		indicesBufferObject = createBufferObject(renderer, indices, sizeof(indices));
		
		initializedBuffers = SDL_TRUE;
	}
	
	mat4_t modelViewMatrix = m4_translation((vec3_t){0.0f, 0.0f, -22.0f});
	
	drawVerticesFromIndices(renderer, modelViewMatrix, RENDERER_TRIANGLE_MODE, vertexArrayObject, indicesBufferObject, 6, (color4_t){0.0f, 0.0f, 0.0f, 0.7f}, RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA | RENDERER_OPTION_DISABLE_DEPTH_TEST);
}

static void initScene(Renderer *renderer)
{
	loadSceneryTextures(renderer);

	loadTiles();

	initCharacters();

	loadCharacterTextures(renderer);
	buildCharacterModels(renderer);

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
	else
	{
		// If any of the scan code's are unknown, assume they are all invalid and reset them
		// The reason I'm doing this is because I gave development builds that saved SDL2 scan codes,
		// but didn't record the defaults version, which would mean the defaults file would be treated the same
		// as SDL 1.2 versioned file. This is a little robustness check against that case :)
		if (gRedRoverInput.r_id == SDL_SCANCODE_UNKNOWN || gRedRoverInput.l_id == SDL_SCANCODE_UNKNOWN || gRedRoverInput.d_id == SDL_SCANCODE_UNKNOWN || gRedRoverInput.u_id == SDL_SCANCODE_UNKNOWN || gGreenTreeInput.r_id == SDL_SCANCODE_UNKNOWN || gGreenTreeInput.l_id == SDL_SCANCODE_UNKNOWN || gGreenTreeInput.d_id == SDL_SCANCODE_UNKNOWN || gGreenTreeInput.u_id == SDL_SCANCODE_UNKNOWN || gBlueLightningInput.r_id == SDL_SCANCODE_UNKNOWN || gBlueLightningInput.l_id == SDL_SCANCODE_UNKNOWN || gBlueLightningInput.d_id == SDL_SCANCODE_UNKNOWN || gBlueLightningInput.u_id == SDL_SCANCODE_UNKNOWN || gPinkBubbleGumInput.r_id == SDL_SCANCODE_UNKNOWN || gPinkBubbleGumInput.l_id == SDL_SCANCODE_UNKNOWN || gPinkBubbleGumInput.d_id == SDL_SCANCODE_UNKNOWN || gPinkBubbleGumInput.u_id == SDL_SCANCODE_UNKNOWN)
		{
			initInput(&gRedRoverInput, SDL_SCANCODE_B, SDL_SCANCODE_C, SDL_SCANCODE_F, SDL_SCANCODE_V, SDL_SCANCODE_G);
			initInput(&gGreenTreeInput, SDL_SCANCODE_L, SDL_SCANCODE_J, SDL_SCANCODE_I, SDL_SCANCODE_K, SDL_SCANCODE_M);
			initInput(&gBlueLightningInput, SDL_SCANCODE_D, SDL_SCANCODE_A, SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_Z);
			initInput(&gPinkBubbleGumInput, SDL_SCANCODE_RIGHT, SDL_SCANCODE_LEFT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_SPACE);
		}
	}

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

static SDL_bool scanJoyGuidAndDescriptionString(FILE *fp, char *guidBuffer, char *descriptionBuffer)
{
	char tempBuffer[MAX_JOY_GUID_BUFFER_LENGTH + MAX_JOY_DESCRIPTION_BUFFER_LENGTH + 2] = {0};
	if (!scanLineTerminatingString(fp, tempBuffer, sizeof(tempBuffer))) return SDL_FALSE;
	
	size_t length = strlen(tempBuffer);
	if (length <= 3) return SDL_FALSE; // () + space at least
	
	if (tempBuffer[length - 1] != ')') return SDL_FALSE;
	
	char *lastOpenParenPtr = strrchr(tempBuffer, '(');
	if (lastOpenParenPtr == NULL || lastOpenParenPtr >= &tempBuffer[length - 1]) return SDL_FALSE;
	
	if (lastOpenParenPtr - tempBuffer < 1) return SDL_FALSE;
	if (lastOpenParenPtr - tempBuffer - 1 >= MAX_JOY_GUID_BUFFER_LENGTH) return SDL_FALSE;
	
	size_t guidLength = lastOpenParenPtr - tempBuffer - 1;
	memcpy(guidBuffer, tempBuffer, guidLength);
	guidBuffer[guidLength] = '\0';
	
	size_t descriptionLength = tempBuffer + length - lastOpenParenPtr - 2;
	if (descriptionLength >= MAX_JOY_DESCRIPTION_BUFFER_LENGTH) return SDL_FALSE;
	memcpy(descriptionBuffer, lastOpenParenPtr + 1, descriptionLength);
	descriptionBuffer[descriptionLength] = '\0';
	
	return SDL_TRUE;
}

static void setJoystickIdFromGuid(SDL_Joystick **joysticks, int *joystickId, char *guidString)
{
	for (int joystickIndex = 0; joystickIndex < MAX_SUPPORTED_JOYSTICKS; joystickIndex++)
	{
		SDL_Joystick *joystick = joysticks[joystickIndex];
		if (joystick != NULL && SDL_JoystickGetAttached(joystick))
		{
			SDL_JoystickGUID guid = SDL_JoystickGetGUID(joystick);
			
			char guidBuffer[MAX_JOY_GUID_BUFFER_LENGTH] = {0};
			SDL_JoystickGetGUIDString(guid, guidBuffer, MAX_JOY_GUID_BUFFER_LENGTH);
			
			if (strncmp(guidString, guidBuffer, MAX_JOY_GUID_BUFFER_LENGTH) == 0)
			{
				*joystickId = SDL_JoystickInstanceID(joystick);
				return;
			}
		}
	}
	
	*joystickId = -1;
}

// SDL 1.2 had a different mapping of key codes compared to SDL 2
// We need to read user defaults from SDL 1.2 key codes once for migration purposes
static SDL_Scancode scanCodeFromKeyCode1_2(int keyCode)
{
	switch (keyCode)
	{
		case 0: return SDL_SCANCODE_UNKNOWN;
		case 8: return SDL_SCANCODE_BACKSPACE;
		case 9: return SDL_SCANCODE_TAB;
		case 12: return SDL_SCANCODE_CLEAR;
		case 13: return SDL_SCANCODE_RETURN;
		case 19: return SDL_SCANCODE_PAUSE;
		case 27: return SDL_SCANCODE_ESCAPE;
		case 32: return SDL_SCANCODE_SPACE;
		case 35: return SDL_SCANCODE_KP_HASH;
		case 38: return SDL_SCANCODE_KP_AMPERSAND;
		case 40: return SDL_SCANCODE_KP_LEFTPAREN;
		case 41: return SDL_SCANCODE_KP_RIGHTPAREN;
		case 43: return SDL_SCANCODE_KP_PLUS;
		case 44: return SDL_SCANCODE_COMMA;
		case 45: return SDL_SCANCODE_MINUS;
		case 46: return SDL_SCANCODE_PERIOD;
		case 47: return SDL_SCANCODE_SLASH;
		case 48: return SDL_SCANCODE_0;
		case 49: return SDL_SCANCODE_1;
		case 50: return SDL_SCANCODE_2;
		case 51: return SDL_SCANCODE_3;
		case 52: return SDL_SCANCODE_4;
		case 53: return SDL_SCANCODE_5;
		case 54: return SDL_SCANCODE_6;
		case 55: return SDL_SCANCODE_7;
		case 56: return SDL_SCANCODE_8;
		case 57: return SDL_SCANCODE_9;
		case 58: return SDL_SCANCODE_KP_COLON;
		case 59: return SDL_SCANCODE_SEMICOLON;
		case 60: return SDL_SCANCODE_KP_LESS;
		case 61: return SDL_SCANCODE_EQUALS;
		case 62: return SDL_SCANCODE_KP_GREATER;
		case 64: return SDL_SCANCODE_KP_AT;
		case 91: return SDL_SCANCODE_LEFTBRACKET;
		case 92: return SDL_SCANCODE_BACKSLASH;
		case 93: return SDL_SCANCODE_RIGHTBRACKET;
		case 97: return SDL_SCANCODE_A;
		case 98: return SDL_SCANCODE_B;
		case 99: return SDL_SCANCODE_C;
		case 100: return SDL_SCANCODE_D;
		case 101: return SDL_SCANCODE_E;
		case 102: return SDL_SCANCODE_F;
		case 103: return SDL_SCANCODE_G;
		case 104: return SDL_SCANCODE_H;
		case 105: return SDL_SCANCODE_I;
		case 106: return SDL_SCANCODE_J;
		case 107: return SDL_SCANCODE_K;
		case 108: return SDL_SCANCODE_L;
		case 109: return SDL_SCANCODE_M;
		case 110: return SDL_SCANCODE_N;
		case 111: return SDL_SCANCODE_O;
		case 112: return SDL_SCANCODE_P;
		case 113: return SDL_SCANCODE_Q;
		case 114: return SDL_SCANCODE_R;
		case 115: return SDL_SCANCODE_S;
		case 116: return SDL_SCANCODE_T;
		case 117: return SDL_SCANCODE_U;
		case 118: return SDL_SCANCODE_V;
		case 119: return SDL_SCANCODE_W;
		case 120: return SDL_SCANCODE_X;
		case 121: return SDL_SCANCODE_Y;
		case 122: return SDL_SCANCODE_Z;
		case 127: return SDL_SCANCODE_DELETE;
		case 256: return SDL_SCANCODE_KP_0;
		case 257: return SDL_SCANCODE_KP_1;
		case 258: return SDL_SCANCODE_KP_2;
		case 259: return SDL_SCANCODE_KP_3;
		case 260: return SDL_SCANCODE_KP_4;
		case 261: return SDL_SCANCODE_KP_5;
		case 262: return SDL_SCANCODE_KP_6;
		case 263: return SDL_SCANCODE_KP_7;
		case 264: return SDL_SCANCODE_KP_8;
		case 265: return SDL_SCANCODE_KP_9;
		case 266: return SDL_SCANCODE_KP_PERIOD;
		case 267: return SDL_SCANCODE_KP_DIVIDE;
		case 268: return SDL_SCANCODE_KP_MULTIPLY;
		case 269: return SDL_SCANCODE_KP_MINUS;
		case 270: return SDL_SCANCODE_KP_PLUS;
		case 271: return SDL_SCANCODE_KP_ENTER;
		case 272: return SDL_SCANCODE_KP_EQUALS;
		case 273: return SDL_SCANCODE_UP;
		case 274: return SDL_SCANCODE_DOWN;
		case 275: return SDL_SCANCODE_RIGHT;
		case 276: return SDL_SCANCODE_LEFT;
		case 277: return SDL_SCANCODE_INSERT;
		case 278: return SDL_SCANCODE_HOME;
		case 279: return SDL_SCANCODE_END;
		case 280: return SDL_SCANCODE_PAGEUP;
		case 281: return SDL_SCANCODE_PAGEDOWN;
		case 282: return SDL_SCANCODE_F1;
		case 283: return SDL_SCANCODE_F2;
		case 284: return SDL_SCANCODE_F3;
		case 285: return SDL_SCANCODE_F4;
		case 286: return SDL_SCANCODE_F5;
		case 287: return SDL_SCANCODE_F6;
		case 288: return SDL_SCANCODE_F7;
		case 289: return SDL_SCANCODE_F8;
		case 290: return SDL_SCANCODE_F9;
		case 291: return SDL_SCANCODE_F10;
		case 292: return SDL_SCANCODE_F11;
		case 293: return SDL_SCANCODE_F12;
		case 294: return SDL_SCANCODE_F13;
		case 295: return SDL_SCANCODE_F14;
		case 296: return SDL_SCANCODE_F15;
		case 301: return SDL_SCANCODE_CAPSLOCK;
		case 302: return SDL_SCANCODE_SCROLLLOCK;
		case 303: return SDL_SCANCODE_RSHIFT;
		case 304: return SDL_SCANCODE_LSHIFT;
		case 305: return SDL_SCANCODE_RCTRL;
		case 306: return SDL_SCANCODE_LCTRL;
		case 307: return SDL_SCANCODE_RALT;
		case 308: return SDL_SCANCODE_LALT;
		case 309: return SDL_SCANCODE_RGUI;
		case 310: return SDL_SCANCODE_LGUI;
		case 313: return SDL_SCANCODE_MODE;
		case 314: return SDL_SCANCODE_APPLICATION;
		case 315: return SDL_SCANCODE_HELP;
		case 316: return SDL_SCANCODE_PRINTSCREEN;
		case 317: return SDL_SCANCODE_SYSREQ;
		case 319: return SDL_SCANCODE_MENU;
		case 320: return SDL_SCANCODE_POWER;
		case 322: return SDL_SCANCODE_UNDO;
	}
	return SDL_SCANCODE_UNKNOWN;
}

static SDL_bool readCharacterInputDefaults(FILE *fp, const char *characterName, Input *input, SDL_Joystick **joysticks, int defaultsVersion)
{
	SDL_bool readInputDefaults = SDL_FALSE;
	
	input->joy_right = calloc(MAX_JOY_DESCRIPTION_BUFFER_LENGTH, 1);
	input->joy_left = calloc(MAX_JOY_DESCRIPTION_BUFFER_LENGTH, 1);
	input->joy_up = calloc(MAX_JOY_DESCRIPTION_BUFFER_LENGTH, 1);
	input->joy_down = calloc(MAX_JOY_DESCRIPTION_BUFFER_LENGTH, 1);
	input->joy_weap = calloc(MAX_JOY_DESCRIPTION_BUFFER_LENGTH, 1);
	
	input->joy_right_guid = calloc(MAX_JOY_GUID_BUFFER_LENGTH, 1);
	input->joy_left_guid = calloc(MAX_JOY_GUID_BUFFER_LENGTH, 1);
	input->joy_up_guid = calloc(MAX_JOY_GUID_BUFFER_LENGTH, 1);
	input->joy_down_guid = calloc(MAX_JOY_GUID_BUFFER_LENGTH, 1);
	input->joy_weap_guid = calloc(MAX_JOY_GUID_BUFFER_LENGTH, 1);
	
	if (!scanExpectedString(fp, characterName)) goto read_input_cleanup;
	if (fscanf(fp, " key right: %i\n", &input->r_id) < 1) goto read_input_cleanup;
	if (defaultsVersion <= 1)
	{
		input->r_id = scanCodeFromKeyCode1_2(input->r_id);
	}
	
	if (!scanExpectedString(fp, characterName)) goto read_input_cleanup;
	if (fscanf(fp, " key left: %i\n", &input->l_id) < 1) goto read_input_cleanup;
	if (defaultsVersion <= 1)
	{
		input->l_id = scanCodeFromKeyCode1_2(input->l_id);
	}
	
	if (!scanExpectedString(fp, characterName)) goto read_input_cleanup;
	if (fscanf(fp, " key up: %i\n", &input->u_id) < 1) goto read_input_cleanup;
	if (defaultsVersion <= 1)
	{
		input->u_id = scanCodeFromKeyCode1_2(input->u_id);
	}
	
	if (!scanExpectedString(fp, characterName)) goto read_input_cleanup;
	if (fscanf(fp, " key down: %i\n", &input->d_id) < 1) goto read_input_cleanup;
	if (defaultsVersion <= 1)
	{
		input->d_id = scanCodeFromKeyCode1_2(input->d_id);
	}
	
	if (!scanExpectedString(fp, characterName)) goto read_input_cleanup;
	if (fscanf(fp, " key weapon: %i\n", &input->weap_id) < 1) goto read_input_cleanup;
	if (defaultsVersion <= 1)
	{
		input->weap_id = scanCodeFromKeyCode1_2(input->weap_id);
	}
	
	if (!scanExpectedString(fp, characterName)) goto read_input_cleanup;
	if (defaultsVersion < 3)
	{
		if (fscanf(fp, " joy right id: %i, axis: %i, joy id: ", &input->rjs_id, &input->rjs_axis_id) < 2) goto read_input_cleanup;
	}
	else
	{
		input->rjs_hat_id = JOY_HAT_NONE;
		if (fscanf(fp, " joy right id: %i, axis: %i, hat: %i, joy id: ", &input->rjs_id, &input->rjs_axis_id, &input->rjs_hat_id) < 3) goto read_input_cleanup;
	}
	if (!scanJoyGuidAndDescriptionString(fp, input->joy_right_guid, input->joy_right)) goto read_input_cleanup;
	setJoystickIdFromGuid(joysticks, &input->joy_right_id, input->joy_right_guid);
	
	if (!scanExpectedString(fp, characterName)) goto read_input_cleanup;
	if (defaultsVersion < 3)
	{
		if (fscanf(fp, " joy left id: %i, axis: %i, joy id: ", &input->ljs_id, &input->ljs_axis_id) < 2) goto read_input_cleanup;
	}
	else
	{
		input->ljs_hat_id = JOY_HAT_NONE;
		if (fscanf(fp, " joy left id: %i, axis: %i, hat: %i, joy id: ", &input->ljs_id, &input->ljs_axis_id, &input->ljs_hat_id) < 3) goto read_input_cleanup;
	}
	if (!scanJoyGuidAndDescriptionString(fp, input->joy_left_guid, input->joy_left)) goto read_input_cleanup;
	setJoystickIdFromGuid(joysticks, &input->joy_left_id, input->joy_left_guid);
	
	if (!scanExpectedString(fp, characterName)) goto read_input_cleanup;
	if (defaultsVersion < 3)
	{
		if (fscanf(fp, " joy up id: %i, axis: %i, joy id: ", &input->ujs_id, &input->ujs_axis_id) < 2) goto read_input_cleanup;
	}
	else
	{
		input->ujs_hat_id = JOY_HAT_NONE;
		if (fscanf(fp, " joy up id: %i, axis: %i, hat: %i, joy id: ", &input->ujs_id, &input->ujs_axis_id, &input->ujs_hat_id) < 3) goto read_input_cleanup;
	}
	if (!scanJoyGuidAndDescriptionString(fp, input->joy_up_guid, input->joy_up)) goto read_input_cleanup;
	setJoystickIdFromGuid(joysticks, &input->joy_up_id, input->joy_up_guid);
	
	if (!scanExpectedString(fp, characterName)) goto read_input_cleanup;
	if (defaultsVersion < 3)
	{
		if (fscanf(fp, " joy down id: %i, axis: %i, joy id: ", &input->djs_id, &input->djs_axis_id) < 2) goto read_input_cleanup;
	}
	else
	{
		input->djs_hat_id = JOY_HAT_NONE;
		if (fscanf(fp, " joy down id: %i, axis: %i, hat: %i, joy id: ", &input->djs_id, &input->djs_axis_id, &input->djs_hat_id) < 3) goto read_input_cleanup;
	}
	if (!scanJoyGuidAndDescriptionString(fp, input->joy_down_guid, input->joy_down)) goto read_input_cleanup;
	setJoystickIdFromGuid(joysticks, &input->joy_down_id, input->joy_down_guid);
	
	if (!scanExpectedString(fp, characterName)) goto read_input_cleanup;
	if (defaultsVersion < 3)
	{
		if (fscanf(fp, " joy weapon id: %i, axis: %i, joy id: ", &input->weapjs_id, &input->weapjs_axis_id) < 2) goto read_input_cleanup;
	}
	else
	{
		input->weapjs_hat_id = JOY_HAT_NONE;
		if (fscanf(fp, " joy weapon id: %i, axis: %i, hat: %i, joy id: ", &input->weapjs_id, &input->weapjs_axis_id, &input->weapjs_hat_id) < 3) goto read_input_cleanup;
	}
	if (!scanJoyGuidAndDescriptionString(fp, input->joy_weap_guid, input->joy_weap)) goto read_input_cleanup;
	setJoystickIdFromGuid(joysticks, &input->joy_weap_id, input->joy_weap_guid);
	
	if (!scanExpectedString(fp, "\n")) goto read_input_cleanup;
	
	readInputDefaults = SDL_TRUE;
	
read_input_cleanup:
	if (!readInputDefaults)
	{
		free(input->joy_right);
		input->joy_right = NULL;
		
		free(input->joy_left);
		input->joy_left = NULL;
		
		free(input->joy_up);
		input->joy_up = NULL;
		
		free(input->joy_down);
		input->joy_down = NULL;
		
		free(input->joy_weap);
		input->joy_weap = NULL;
		
		free(input->joy_right_guid);
		input->joy_right_guid = NULL;
		
		free(input->joy_left_guid);
		input->joy_left_guid = NULL;
		
		free(input->joy_up_guid);
		input->joy_up_guid = NULL;
		
		free(input->joy_down_guid);
		input->joy_down_guid = NULL;
		
		free(input->joy_weap_guid);
		input->joy_weap_guid = NULL;
	}
	
	return readInputDefaults;
}

static void readDefaults(SDL_Joystick **joysticks)
{
	#ifndef WINDOWS
		// this would be a good time to get the default user name
		getDefaultUserName(gUserNameString, MAX_USER_NAME_SIZE - 1);
		gUserNameStringIndex = strlen(gUserNameString);
	#endif

	FILE *fp = getUserDataFile("rb");

	if (fp == NULL)
	{
		return;
	}
	
	// Read defaults version at the end of the file if present
	int defaultsVersion = 0;
	if (fseek(fp, -20, SEEK_END) < 0) goto cleanup;
	if (fscanf(fp, "Defaults version: %i\n", &defaultsVersion) < 1)
	{
		// If no defaults version is available, assume this is the first version
		// (where no defaults version was specified in the file)
		defaultsVersion = 1;
	}
	
	if (fseek(fp, 0, SEEK_SET) < 0) goto cleanup;

	// conole flag default
	int consoleFlag = 0;
	if (fscanf(fp, "Console flag: %i\n\n", &consoleFlag) < 1) goto cleanup;
	gConsoleFlag = (consoleFlag != 0);

	// video defaults
	int windowWidth = 0;
	if (fscanf(fp, "screen width: %i\n", &windowWidth) < 1) goto cleanup;
	
	int windowHeight = 0;
	if (fscanf(fp, "screen height: %i\n", &windowHeight) < 1) goto cleanup;
	
	if (defaultsVersion < 2 || windowWidth <= 0 || windowHeight <= 0)
	{
		gWindowWidth = 800;
		gWindowHeight = 500;
	}
	else
	{
		gWindowWidth = windowWidth;
		gWindowHeight = windowHeight;
	}
	
	int vsyncFlag = 0;
	if (fscanf(fp, "vsync flag: %i\n", &vsyncFlag) < 1) goto cleanup;
	
	if (defaultsVersion < 2)
	{
		gVsyncFlag = (vsyncFlag != 0);
	}
	else
	{
		gVsyncFlag = SDL_TRUE;
	}
	
	int fpsFlag = 0;
	if (fscanf(fp, "fps flag: %i\n", &fpsFlag) < 1) goto cleanup;
	
	int aaFlag = 0;
	if (fscanf(fp, "FSAA flag: %i\n", &aaFlag) < 1) goto cleanup;
	
	if (defaultsVersion < 2)
	{
		gFsaaFlag = SDL_TRUE;
	}
	else
	{
		gFsaaFlag = (aaFlag != 0);
	}

	int fullscreenFlag = 0;
	if (fscanf(fp, "Fullscreen flag: %d\n", &fullscreenFlag) < 1) goto cleanup;
	if (defaultsVersion < 2)
	{
		gFullscreenFlag = SDL_FALSE;
	}
	else
	{
		gFullscreenFlag = (fullscreenFlag != 0);
	}

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
	
	if (!readCharacterInputDefaults(fp, "Pink Bubblegum", &gPinkBubbleGumInput, joysticks, defaultsVersion)) goto cleanup;
	if (!readCharacterInputDefaults(fp, "Red Rover", &gRedRoverInput, joysticks, defaultsVersion)) goto cleanup;
	if (!readCharacterInputDefaults(fp, "Green Tree", &gGreenTreeInput, joysticks, defaultsVersion)) goto cleanup;
	if (!readCharacterInputDefaults(fp, "Blue Lightning", &gBlueLightningInput, joysticks, defaultsVersion)) goto cleanup;
	
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

static char *joystickGuidFromId(char *guid)
{
	if (guid != NULL && strlen(guid) > 0)
	{
		return guid;
	}
	else
	{
		return "_NULL_";
	}
}

static void writeCharacterInput(FILE *fp, const char *characterName, Input *input)
{
	fprintf(fp, "%s key right: %i\n", characterName, input->r_id);
	fprintf(fp, "%s key left: %i\n", characterName, input->l_id);
	fprintf(fp, "%s key up: %i\n", characterName, input->u_id);
	fprintf(fp, "%s key down: %i\n", characterName, input->d_id);
	fprintf(fp, "%s key weapon: %i\n", characterName, input->weap_id);
	
	fprintf(fp, "\n");
	
	fprintf(fp, "%s joy right id: %i, axis: %i, hat: %i, joy id: %s (%s)\n", characterName, input->rjs_id, input->rjs_axis_id, input->rjs_hat_id, joystickGuidFromId(input->joy_right_guid), input->joy_right);
	fprintf(fp, "%s joy left id: %i, axis: %i, hat: %i, joy id: %s (%s)\n", characterName, input->ljs_id, input->ljs_axis_id, input->ljs_hat_id, joystickGuidFromId(input->joy_left_guid), input->joy_left);
	fprintf(fp, "%s joy up id: %i, axis: %i, hat: %i, joy id: %s (%s)\n", characterName, input->ujs_id, input->ujs_axis_id, input->ujs_hat_id, joystickGuidFromId(input->joy_up_guid), input->joy_up);
	fprintf(fp, "%s joy down id: %i, axis: %i, hat: %i, joy id: %s (%s)\n", characterName, input->djs_id, input->djs_axis_id, input->djs_hat_id, joystickGuidFromId(input->joy_down_guid), input->joy_down);
	fprintf(fp, "%s joy weapon id: %i, axis: %i, hat: %i, joy id: %s (%s)\n", characterName, input->weapjs_id, input->weapjs_axis_id, input->weapjs_hat_id, joystickGuidFromId(input->joy_weap_guid), input->joy_weap);
}

static void writeDefaults(Renderer *renderer)
{
	FILE *fp = getUserDataFile("wb");

	if (fp == NULL)
	{
		fprintf(stderr, "writeDefaults: file pointer is NULL\n");
		return;
	}

	// conole flag default
	fprintf(fp, "Console flag: %i\n\n", gConsoleFlag);

	// video defaults
	fprintf(fp, "screen width: %i\n", renderer->windowWidth);
	fprintf(fp, "screen height: %i\n", renderer->windowHeight);
	
	fprintf(fp, "vsync flag: %i\n", gVsyncFlag);
	
	fprintf(fp, "fps flag: %i\n", gFpsFlag);
	fprintf(fp, "FSAA flag: %i\n", gFsaaFlag);
	fprintf(fp, "Fullscreen flag: %i\n", renderer->fullscreen);

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

	// Character defaults
	
	writeCharacterInput(fp, "Pink Bubblegum", &gPinkBubbleGumInput);
	fprintf(fp, "\n");
	
	writeCharacterInput(fp, "Red Rover", &gRedRoverInput);
	fprintf(fp, "\n");
	
	writeCharacterInput(fp, "Green Tree", &gGreenTreeInput);
	fprintf(fp, "\n");
	
	writeCharacterInput(fp, "Blue Lightning", &gBlueLightningInput);
	
	fprintf(fp, "\n");
	fprintf(fp, "Server IP Address: %s\n", gServerAddressString);

	fprintf(fp, "\n");
	fprintf(fp, "Net name: %s\n", gUserNameString);
	
	fprintf(fp, "\n");
	fprintf(fp, "Audio effects: %i\n", gAudioEffectsFlag);
	fprintf(fp, "Music: %i\n", gAudioMusicFlag);
	
	// If defaults version ever gets > 9, I may have to adjust the defaults reading code
	// I doubt this will ever happen though
	fprintf(fp, "\nDefaults version: 3\n");

	fclose(fp);
}

void initGame(void)
{
	loadTiles();

	loadCharacter(&gRedRover);
	loadCharacter(&gGreenTree);
	loadCharacter(&gPinkBubbleGum);
	loadCharacter(&gBlueLightning);

	startAnimation();
	
	int initialNumberOfLives;
	if (gNetworkConnection != NULL && gNetworkConnection->type == NETWORK_SERVER_TYPE)
	{
		initialNumberOfLives = gCharacterNetLives;
	}
	else if (gNetworkConnection != NULL && gNetworkConnection->type == NETWORK_CLIENT_TYPE)
	{
		initialNumberOfLives = gNetworkConnection->characterLives;
	}
	else
	{
		initialNumberOfLives = gCharacterLives;
	}

	gRedRover.lives = initialNumberOfLives;
	gGreenTree.lives = initialNumberOfLives;
	gPinkBubbleGum.lives = initialNumberOfLives;
	gBlueLightning.lives = initialNumberOfLives;

	gGameState = GAME_STATE_ON;

	// wait for NEW_GAME_WILL_BEGIN_DELAY seconds until the game can be started.
	gGameStartNumber = NEW_GAME_WILL_BEGIN_DELAY;
	
	gEscapeHeldDownTimer = 0;
}

void endGame(void)
{
	gGameHasStarted = SDL_FALSE;

	endAnimation();

	gGameState = GAME_STATE_OFF;

	gGameWinner = NO_CHARACTER;
	gGameShouldReset = SDL_FALSE;
}

static void exitGame(void)
{
	resetCharacterWins();
	endGame();
	
	if (gNetworkConnection)
	{
		if (gNetworkConnection->type == NETWORK_SERVER_TYPE)
		{
			GameMessage message;
			message.type = QUIT_MESSAGE_TYPE;
			sendToClients(0, &message);
		}
		else if (gNetworkConnection->type == NETWORK_CLIENT_TYPE)
		{
			GameMessage message;
			message.type = QUIT_MESSAGE_TYPE;
			sendToServer(message);
		}
	}
}

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
		mat4_t modelViewMatrix = m4_translation((vec3_t){6.48f, 6.48f, -18.0f});
		drawString(renderer, modelViewMatrix, (color4_t){0.0f, 0.5f, 0.8f, 1.0f}, 0.16f * length, 0.5f, fpsString);
	}
}

static void drawPings(Renderer *renderer)
{
	if (gNetworkConnection != NULL)
	{
		double currentTime = SDL_GetTicks() / 1000.0;
		static double lastPingDisplayTime = -1.0;
		
		if (gNetworkConnection->type == NETWORK_CLIENT_TYPE)
		{
			static char pingString[256] = {0};
			
			Character *character = gNetworkConnection->character;
			
			if (character != NULL && gNetworkConnection->serverHalfPing != 0)
			{
				mat4_t modelViewMatrix = m4_translation((vec3_t){6.48f, 5.48f, -18.0f});
				
				if (lastPingDisplayTime < 0.0 || currentTime - lastPingDisplayTime >= 1.0)
				{
					snprintf(pingString, sizeof(pingString), "%u", gNetworkConnection->serverHalfPing * 2);
					
					lastPingDisplayTime = currentTime;
				}
				
				int length = strlen(pingString);
				if (length > 0)
				{
					drawString(renderer, modelViewMatrix, (color4_t){character->red, character->green, character->blue, 1.0f}, 0.16f * length, 0.5f, pingString);
				}
			}
		}
		else if (gNetworkConnection->type == NETWORK_SERVER_TYPE)
		{
			static char pingStrings[3][256] = {{0}, {0}, {0}};
			
			SDL_bool rebuildStrings = (lastPingDisplayTime < 0.0 || currentTime - lastPingDisplayTime >= 1.0);
			
			for (uint32_t pingAddressIndex = 0; pingAddressIndex < 3; pingAddressIndex++)
			{
				if (gNetworkConnection->clientHalfPings[pingAddressIndex] != 0)
				{
					Character *character = getCharacter(pingAddressIndex + 1);
					
					mat4_t modelViewMatrix = m4_translation((vec3_t){6.48f, 5.48f - pingAddressIndex * 1.0f, -18.0f});
					
					if (rebuildStrings)
					{
						snprintf(pingStrings[pingAddressIndex], sizeof(pingStrings[pingAddressIndex]), "%u", gNetworkConnection->clientHalfPings[pingAddressIndex] * 2);
						
						lastPingDisplayTime = currentTime;
					}
					
					int length = strlen(pingStrings[pingAddressIndex]);
					if (length > 0)
					{
						drawString(renderer, modelViewMatrix, (color4_t){character->red, character->green, character->blue, 1.0f}, 0.16f * length, 0.5f, pingStrings[pingAddressIndex]);
					}
				}
			}
		}
	}
}

static void drawScoreboardTextForCharacter(Renderer *renderer, Character *character, mat4_t iconModelViewMatrix)
{
	color4_t characterColor = (color4_t){character->red, character->green, character->blue, 0.7f};

	char buffer[256] = {0};

	mat4_t winsLabelModelViewMatrix = m4_mul(iconModelViewMatrix, m4_translation((vec3_t){0.225f, -1.6f, 0.0f}));

	drawStringScaled(renderer, winsLabelModelViewMatrix, characterColor, 0.0024f, "Wins:");

	mat4_t winsModelViewMatrix = m4_mul(winsLabelModelViewMatrix, m4_translation((vec3_t){0.87f, 0.0f, 0.0f}));

	snprintf(buffer, sizeof(buffer) - 1, "%d", character->wins);
	drawStringScaled(renderer, winsModelViewMatrix, characterColor, 0.0024f, buffer);

	mat4_t killsLabelModelViewMatrix = m4_mul(winsLabelModelViewMatrix, m4_translation((vec3_t){0.0f, -1.6f, 0.0f}));

	drawStringScaled(renderer, killsLabelModelViewMatrix, characterColor, 0.0024f, "Kills:");

	mat4_t killsModelViewMatrix = m4_mul(winsModelViewMatrix, m4_translation((vec3_t){0.0f, -1.6f, 0.0f}));

	snprintf(buffer, sizeof(buffer) - 1, "%d", character->kills);
	drawStringScaled(renderer, killsModelViewMatrix, characterColor, 0.0024f, buffer);
}

// We separate drawing icons vs scores on scoreboard
typedef enum
{
	SCOREBOARD_RENDER_ICONS,
	SCOREBOARD_RENDER_SCORES
} ScoreboardRenderType;

static void drawScoreboardForCharacters(Renderer *renderer, ScoreboardRenderType renderType)
{
	mat4_t iconModelViewMatrices[] =
	{
		m4_translation((vec3_t){-6.0f / 1.25f, 7.0f / 1.25f, -25.0f / 1.25f}),
		m4_translation((vec3_t){-2.0f / 1.25f, 7.0f / 1.25f, -25.0f / 1.25f}),
		m4_translation((vec3_t){2.0f / 1.25f, 7.0f / 1.25f, -25.0f / 1.25f}),
		m4_translation((vec3_t){6.0f / 1.25f, 7.0f / 1.25f, -25.0f / 1.25f})
	};
	
	if (renderType == SCOREBOARD_RENDER_ICONS)
	{
		drawCharacterIcons(renderer, iconModelViewMatrices);
	}
	else
	{
		drawScoreboardTextForCharacter(renderer, &gPinkBubbleGum, iconModelViewMatrices[0]);
		drawScoreboardTextForCharacter(renderer, &gRedRover, iconModelViewMatrices[1]);
		drawScoreboardTextForCharacter(renderer, &gGreenTree, iconModelViewMatrices[2]);
		drawScoreboardTextForCharacter(renderer, &gBlueLightning, iconModelViewMatrices[3]);
	}
}

static void drawScene(Renderer *renderer)
{
	if (gGameState == GAME_STATE_ON)
	{
		// Render opaque objects first
		
		// Weapons renders at z = -24.0f to -25.0f after a world rotation
		drawWeapon(renderer, gRedRover.weap);
		drawWeapon(renderer, gGreenTree.weap);
		drawWeapon(renderer, gPinkBubbleGum.weap);
		drawWeapon(renderer, gBlueLightning.weap);

		// Characters renders at z = -25.3 to -24.7 after a world rotation when not fallen
		// When falling, will reach to around -195
		drawCharacters(renderer, RENDERER_OPTION_NONE);

		// Tiles renders at z = -25.0f to -26.0f after a world rotation when not fallen
		drawTiles(renderer);
		
		// Character icons at the bottom of the screen at z = -25.0f
		const mat4_t characterIconTranslations[] =
		{
			m4_translation((vec3_t){CHARACTER_ICON_OFFSET, -9.5f, -25.0f}),
			m4_translation((vec3_t){CHARACTER_ICON_OFFSET + CHARACTER_ICON_DISPLACEMENT, -9.5f, -25.0f}),
			m4_translation((vec3_t){CHARACTER_ICON_OFFSET + CHARACTER_ICON_DISPLACEMENT * 2, -9.5f, -25.0f}),
			m4_translation((vec3_t){CHARACTER_ICON_OFFSET + CHARACTER_ICON_DISPLACEMENT * 3, -9.5f, -25.0f})
		};
		drawCharacterIcons(renderer, characterIconTranslations);
		
		// Render transparent objects from zFar to zNear
		
		// Sky renders at z = -38.0f
		drawSky(renderer, RENDERER_OPTION_BLENDING_ALPHA);
		
		// Characters renders at z = -25.3 to -24.7 after a world rotation when not fallen
		// When falling, will reach to around -195
		drawCharacters(renderer, RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA);
		
		// Character lives at z = -25.0f
		drawAllCharacterLives(renderer, characterIconTranslations);
		
		// Render game instruction and holding escape text at -25.0f
		{
			mat4_t modelViewMatrix = m4_translation((vec3_t){-1.0f / 11.2f, 80.0f / 11.2f, -280.0f / 11.2f});
			
			if (!gGameHasStarted)
			{
				color4_t textColor = (color4_t){0.0f, 0.0f, 1.0f, 1.0f};
				
				if (gPinkBubbleGum.netState == NETWORK_PENDING_STATE || gRedRover.netState == NETWORK_PENDING_STATE || gGreenTree.netState == NETWORK_PENDING_STATE || gBlueLightning.netState == NETWORK_PENDING_STATE)
				{
					if (gNetworkConnection)
					{
						// be sure to take account the plural form of player(s)
						if (gNetworkConnection->numberOfPlayersToWaitFor > 1)
						{
							char buffer[256] = {0};
							snprintf(buffer, sizeof(buffer) - 1, "Waiting for %d players to connect...", gNetworkConnection->numberOfPlayersToWaitFor);
							
							mat4_t leftAlignedModelViewMatrix = m4_mul(m4_translation((vec3_t){-6.8f, 0.0f, 0.0f}), modelViewMatrix);
							
							drawStringLeftAligned(renderer, leftAlignedModelViewMatrix, textColor, 0.0045538f, buffer);
						}
						else if (gNetworkConnection->numberOfPlayersToWaitFor == 0)
						{
							mat4_t leftAlignedModelViewMatrix = m4_mul(m4_translation((vec3_t){-6.8f, 0.0f, 0.0f}), modelViewMatrix);
							
							drawStringLeftAligned(renderer, leftAlignedModelViewMatrix, textColor, 0.0045538f, "Waiting for players to connect...");
						}
						else
						{
							mat4_t leftAlignedModelViewMatrix = m4_mul(m4_translation((vec3_t){-6.8f, 0.0f, 0.0f}), modelViewMatrix);
							
							drawStringLeftAligned(renderer, leftAlignedModelViewMatrix, textColor, 0.0045538f, "Waiting for 1 player to connect...");
						}
					}
				}
				
				else if (gGameStartNumber > 0)
				{
					char buffer[256] = {0};
					snprintf(buffer, sizeof(buffer) - 1, "Game begins in %d", gGameStartNumber);
					
					mat4_t leftAlignedModelViewMatrix = m4_mul(m4_translation((vec3_t){-3.8f, 0.0f, 0.0f}), modelViewMatrix);
					drawStringLeftAligned(renderer, leftAlignedModelViewMatrix, textColor, 0.0045538f, buffer);
				}
				
				else if (gGameStartNumber == 0)
				{
					gGameHasStarted = SDL_TRUE;
				}
			}
			
			// Render escape game text
			if (gEscapeHeldDownTimer > 0 && gGameWinner == NO_CHARACTER)
			{
				color4_t textColor = (color4_t){1.0f, 0.0f, 0.0f, 1.0f};
				
				char buffer[] = "Hold down Escape to quit...";
				
				mat4_t leftAlignedModelViewMatrix = m4_mul(m4_translation((vec3_t){-4.3f, 1.7f, 0.0f}), modelViewMatrix);
				drawStringLeftAligned(renderer, leftAlignedModelViewMatrix, textColor, 0.0035f, buffer);
			}
		}
		
		if (gConsoleActivated)
		{
			// Console at z = -25.0f
			drawConsole(renderer);
		}
		
		// Winning/Losing text at z = -25.0f
		if (gGameWinner != NO_CHARACTER)
		{
			if (gConsoleActivated)
			{
				// Console text at z = -23.0f
				drawConsoleText(renderer);
			}
			
			// Renders at z = -22.0f
			drawBlackBox(renderer);
			
			if (gGameWinner != NO_CHARACTER)
			{
				// Character icons on scoreboard at z = -20.0f
				// This is supposed to actually be opaque, but it doesn't render properly in the GL renderer
				// if this is rendered beforehand, don't know why.
				drawScoreboardForCharacters(renderer, SCOREBOARD_RENDER_ICONS);
			}
			
			// Renders winning/losing text at z = -20.0f
			mat4_t winLoseModelViewMatrix = m4_translation((vec3_t){70.0f / 14.0f, 100.0f / 14.0f, -280.0f / 14.0f});
			
			if (gGameWinner == RED_ROVER)
			{
				char winBuffer[128] = {0};
				snprintf(winBuffer, sizeof(winBuffer) - 1, "%s wins!", gRedRover.netName != NULL ? gRedRover.netName : "Red Rover");
				
				drawStringScaled(renderer, winLoseModelViewMatrix, (color4_t){gRedRover.red, gRedRover.green, gRedRover.blue, 1.0f}, 0.0027f, winBuffer);
			}
			else if (gGameWinner == GREEN_TREE)
			{
				char winBuffer[128] = {0};
				snprintf(winBuffer, sizeof(winBuffer) - 1, "%s wins!", gGreenTree.netName != NULL ? gGreenTree.netName : "Green Tree");
				
				drawStringScaled(renderer, winLoseModelViewMatrix, (color4_t){gGreenTree.red, gGreenTree.green, gGreenTree.blue, 1.0f}, 0.0027f, winBuffer);
			}
			else if (gGameWinner == PINK_BUBBLE_GUM)
			{
				char winBuffer[128] = {0};
				snprintf(winBuffer, sizeof(winBuffer) - 1, "%s wins!", gPinkBubbleGum.netName != NULL ? gPinkBubbleGum.netName : "Pink Bubblegum");
				
				drawStringScaled(renderer, winLoseModelViewMatrix, (color4_t){gPinkBubbleGum.red, gPinkBubbleGum.green, gPinkBubbleGum.blue, 1.0f}, 0.0027f, winBuffer);
			}
			else if (gGameWinner == BLUE_LIGHTNING)
			{
				char winBuffer[128] = {0};
				snprintf(winBuffer, sizeof(winBuffer) - 1, "%s wins!", gBlueLightning.netName != NULL ? gBlueLightning.netName : "Blue Lightning");
				
				drawStringScaled(renderer, winLoseModelViewMatrix, (color4_t){gBlueLightning.red, gBlueLightning.green, gBlueLightning.blue, 1.0f}, 0.0027f, winBuffer);
			}
			
			// Character scores on scoreboard at z = -20.0f
			drawScoreboardForCharacters(renderer, SCOREBOARD_RENDER_SCORES);
			
			// Play again or exit text at z = -20.0f
			{
				mat4_t modelViewMatrix = m4_translation((vec3_t){0.0f / 1.25f, -7.0f / 1.25f, -25.0f / 1.25f});
				
				if (gNetworkConnection == NULL || gNetworkConnection->type == NETWORK_SERVER_TYPE)
				{
					// Draw a "Press ENTER to play again" notice
					drawStringScaled(renderer, modelViewMatrix, (color4_t){0.0f, 0.0f, 0.4f, 0.6f}, 0.004f, "Fire to play again or Escape to quit");
				}
				else if (gNetworkConnection != NULL && gNetworkConnection->type == NETWORK_CLIENT_TYPE && gEscapeHeldDownTimer > 0)
				{
					drawStringScaled(renderer, modelViewMatrix, (color4_t){0.0f, 0.0f, 0.4f, 0.6f}, 0.004f, "Hold Escape to quit...");
				}
			}
		}
		else
		{
			if (gConsoleActivated)
			{
				// Console text at z =  -24.0f
				drawConsoleText(renderer);
			}
		}
		
		if (gDrawFPS)
		{
			// FPS renders at z = -18.0f
			drawFramesPerSecond(renderer);
		}
		
		if (gDrawPings)
		{
			// Pings render at z = -18.0f
			drawPings(renderer);
		}
	}
	else /* if (gGameState != GAME_STATE_ON) */
	{
		// The game title and menu's should be up front the most
		// The black box should be behind the title and menu's
		// The sky should be behind the black box
		
		// Sky renders at -38.0f
		drawSky(renderer, RENDERER_OPTION_DISABLE_DEPTH_TEST);
		
		// Black box renders at -22.0f
		drawBlackBox(renderer);
		
		// Title renders at -20.0f
		mat4_t gameTitleModelViewMatrix = m4_translation((vec3_t){-0.2f, 5.4f, -20.0f});
		drawStringScaled(renderer, gameTitleModelViewMatrix, (color4_t){0.3f, 0.2f, 1.0f, 0.4f}, 0.00592f, "Sky Checkers");
		
		if (gGameState == GAME_STATE_CONNECTING)
		{
			// Rendering connecting to server at z = -20.0f
			mat4_t translationMatrix = m4_translation((vec3_t){-1.0f / 14.0f, 15.0f / 14.0f, -280.0f / 14.0f});
			color4_t textColor = (color4_t){0.3f, 0.2f, 1.0f, 1.0f};
			
			drawString(renderer, translationMatrix, textColor, 50.0f / 14.0f, 5.0f / 14.0f, "Connecting to server...");
		}
		else /* if (gGameState == GAME_STATE_OFF) */
		{
			// Menus render at z = -20.0f
			drawMenus(renderer);
			
			if (isChildBeingDrawn(&gJoyStickConfig[0][1]) /* pinkBubbleGumConfigRightJoyStick */	||
				isChildBeingDrawn(&gJoyStickConfig[1][1]) /* redRoverConfigRightJoyStick */			||
				isChildBeingDrawn(&gJoyStickConfig[2][1]) /* greenTreeRightgJoyStickConfig */		||
				isChildBeingDrawn(&gJoyStickConfig[3][1]) /* blueLightningConfigJoyStick */)
			{
				mat4_t translationMatrix = m4_translation((vec3_t){-1.0f / 14.0f, 50.0f / 14.0f, -280.0f / 14.0f});
				
				color4_t textColor = (color4_t){0.3f, 0.2f, 1.0f, 1.0f};
				
				drawString(renderer, translationMatrix, textColor, 100.0f / 14.0f, 5.0f / 14.0f, "Enter: map controller input; Escape: cancel; Spacebar: clear.");
				
				mat4_t noticeModelViewMatrix = m4_mul(translationMatrix, m4_translation((vec3_t){0.0f / 14.0f, -20.0f / 14.0f, 0.0f / 14.0f}));
				
				drawString(renderer, noticeModelViewMatrix, textColor, 50.0f / 16.0f, 5.0f / 16.0f, "(Controllers only work in-game)");
			}
			
			else if (isChildBeingDrawn(&gCharacterConfigureKeys[0][1]) /* pinkBubbleGum */	||
					 isChildBeingDrawn(&gCharacterConfigureKeys[1][1]) /* redRover */		||
					 isChildBeingDrawn(&gCharacterConfigureKeys[2][1]) /* greenTree */		||
					 isChildBeingDrawn(&gCharacterConfigureKeys[3][1]) /* blueLightning */)
			{
				mat4_t translationMatrix = m4_translation((vec3_t){-1.0f / 14.0f, 50.0f / 14.0f, -280.0f / 14.0f});
				
				color4_t textColor = (color4_t){0.3f, 0.2f, 1.0f, 1.0f};
				
				drawString(renderer, translationMatrix, textColor, 70.0f / 14.0f, 5.0f / 14.0f, "Enter: map keyboard input; Escape: cancel.");
			}
		}
		
		if (gDrawFPS)
		{
			// FPS renders at z = -18.0f
			drawFramesPerSecond(renderer);
		}
	}
}

static void writeTextInput(const char *text, uint8_t maxSize)
{
	if (gConsoleActivated || gNetworkAddressFieldIsActive || gNetworkUserNameFieldIsActive)
	{
		for (uint8_t textIndex = 0; textIndex < maxSize; textIndex++)
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
}

static void eventInput(SDL_Event *event, Renderer *renderer, SDL_bool *needsToDrawScene, SDL_Joystick **joysticks, SDL_bool *quit)
{
	SDL_Window *window = renderer->window;
	
#ifdef MAC_OS_X
	SDL_Keymod metaMod = (KMOD_LGUI | KMOD_RGUI);
#else
	SDL_Keymod metaMod = (KMOD_LCTRL | KMOD_RCTRL);
#endif
	
	switch (event->type)
	{
		case SDL_KEYDOWN:
			if (event->key.keysym.sym == SDLK_v && (event->key.keysym.mod & metaMod) != 0 && SDL_HasClipboardText())
			{
				char *clipboardText = SDL_GetClipboardText();
				if (clipboardText != NULL)
				{
					writeTextInput(clipboardText, 128);
				}
			}
#ifndef MAC_OS_X
			else if (event->key.keysym.sym == SDLK_RETURN && (event->key.keysym.mod & KMOD_ALT) != 0)
			{
#ifdef WINDOWS
				if (!renderer->windowsNativeFullscreenToggling)
#endif
				{
					uint32_t windowFlags = SDL_GetWindowFlags(renderer->window);
					if ((windowFlags & (SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_FULLSCREEN)) == 0)
					{
						if (SDL_SetWindowFullscreen(renderer->window, SDL_WINDOW_FULLSCREEN_DESKTOP) != 0)
						{
							fprintf(stderr, "Failed to set fullscreen because: %s\n", SDL_GetError());
						}
						else
						{
							renderer->fullscreen = SDL_TRUE;
						}
					}
					else
					{
						if (SDL_SetWindowFullscreen(renderer->window, 0) != 0)
						{
							fprintf(stderr, "Failed to escape fullscreen because: %s\n", SDL_GetError());
						}
						else
						{
							renderer->fullscreen = SDL_FALSE;
#ifdef WINDOWS
							// Not sure why but on Windows at least a resize event isn't sent when exiting fullscreen
							updateViewport(renderer, renderer->windowWidth, renderer->windowHeight);
#endif
						}
					}
				}
			}
#endif
			else if (!gConsoleActivated && gGameState == GAME_STATE_ON && gGameWinner != NO_CHARACTER &&
				(event->key.keysym.scancode == gPinkBubbleGumInput.weap_id || event->key.keysym.scancode == gRedRoverInput.weap_id ||
				 event->key.keysym.scancode == gBlueLightningInput.weap_id || event->key.keysym.scancode == gGreenTreeInput.weap_id ||
				event->key.keysym.scancode == SDL_SCANCODE_RETURN || event->key.keysym.scancode == SDL_SCANCODE_KP_ENTER))
			{
				// new game
				if (!gNetworkConnection || gNetworkConnection->type == NETWORK_SERVER_TYPE)
				{
					if (gNetworkConnection)
					{
						GameMessage message;
						message.type = GAME_RESET_MESSAGE_TYPE;
						sendToClients(0, &message);
					}
					gGameShouldReset = SDL_TRUE;
				}
			}
			else if (event->key.keysym.sym == SDLK_RETURN || event->key.keysym.sym == SDLK_KP_ENTER)
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

				else if (gGameState == GAME_STATE_OFF)
				{
					invokeMenu(&gGameState);
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

				else if (gGameState == GAME_STATE_OFF)
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

				else if (gGameState == GAME_STATE_OFF)
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
				if (gGameState == GAME_STATE_OFF)
				{
					if (gEscapeHeldDownTimer == 0)
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
				}
				else if (gGameState == GAME_STATE_ON)
				{
					if (gConsoleActivated)
					{
						clearConsole();
					}
					else if (gGameWinner != NO_CHARACTER && (gNetworkConnection == NULL || gNetworkConnection->type == NETWORK_SERVER_TYPE))
					{
						exitGame();
					}
					else if (gEscapeHeldDownTimer == 0)
					{
						gEscapeHeldDownTimer = SDL_GetTicks();
					}
				}
			}

			else if (event->key.keysym.sym == SDLK_BACKQUOTE && gGameState == GAME_STATE_ON)
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
			break;
		case SDL_KEYUP:
			if (event->key.keysym.scancode == SDL_SCANCODE_ESCAPE)
			{
				gEscapeHeldDownTimer = 0;
			}
			break;
		case SDL_TEXTINPUT:
			writeTextInput(event->text.text, SDL_TEXTINPUTEVENT_TEXT_SIZE);
			break;

		case SDL_JOYBUTTONDOWN:
			if (gGameState == GAME_STATE_ON && gGameWinner != NO_CHARACTER && ((SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0) && !joyButtonEventMatchesMovementFromInput(&event->jbutton, &gPinkBubbleGumInput) && !joyButtonEventMatchesMovementFromInput(&event->jbutton, &gRedRoverInput) && !joyButtonEventMatchesMovementFromInput(&event->jbutton, &gGreenTreeInput) && !joyButtonEventMatchesMovementFromInput(&event->jbutton, &gBlueLightningInput))
			{
				// new game
				if (!gConsoleActivated && (!gNetworkConnection || gNetworkConnection->type == NETWORK_SERVER_TYPE))
				{
					if (gNetworkConnection)
					{
						GameMessage message;
						message.type = GAME_RESET_MESSAGE_TYPE;
						sendToClients(0, &message);
					}
					gGameShouldReset = SDL_TRUE;
				}
			}

			break;
		case SDL_JOYDEVICEADDED:
			addJoyStick(joysticks, event->jdevice.which);
			reloadJoySticks(joysticks);
			break;
		case SDL_JOYDEVICEREMOVED:
			removeJoyStick(joysticks, event->jdevice.which);
			reloadJoySticks(joysticks);
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
			else if (event->window.event == SDL_WINDOWEVENT_HIDDEN)
			{
				*needsToDrawScene = SDL_FALSE;
			}
			else if (event->window.event == SDL_WINDOWEVENT_SHOWN)
			{
				*needsToDrawScene = SDL_TRUE;
			}
			else if (event->window.event == SDL_WINDOWEVENT_RESIZED)
			{
				updateViewport(renderer, event->window.data1, event->window.data2);
			}
			break;
		case SDL_QUIT:
			*quit = SDL_TRUE;
			break;
	}
	
	if (gGameState == GAME_STATE_ON && gEscapeHeldDownTimer > 0 && SDL_GetTicks() - gEscapeHeldDownTimer > 700)
	{
		exitGame();
	}

	/*
	 * Perform up and down key/joystick character actions when playing the game
	 * Make sure the user isn't trying to toggle fullscreen.
	 * Other actions, such as changing menus are dealt before here
	 */
	if (!(event->key.keysym.sym == SDLK_RETURN && (SDL_GetModState() & metaMod) != 0) &&
		gGameState == GAME_STATE_ON && (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP || event->type == SDL_JOYBUTTONDOWN || event->type == SDL_JOYHATMOTION || event->type == SDL_JOYBUTTONUP || event->type == SDL_JOYAXISMOTION))
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

#define MAX_ITERATIONS (25 * ANIMATION_TIMER_INTERVAL)

static void eventLoop(Renderer *renderer, SDL_Joystick **joysticks)
{
	SDL_Event event;
	SDL_bool needsToDrawScene = SDL_TRUE;
	SDL_bool done = SDL_FALSE;

	int fps = 30;
	int delay = 1000 / fps;
	int thenTicks = -1;
	int nowTicks;

	while (!done)
	{
		while (SDL_PollEvent(&event))
		{
			eventInput(&event, renderer, &needsToDrawScene, joysticks, &done);
		}
		
		// Update game state
		// http://ludobloom.com/tutorials/timestep.html
		static double lastFrameTime = 0.0;
		static double cyclesLeftOver = 0.0;
		
		double currentTime = SDL_GetTicks() / 1000.0;
		double updateIterations = ((currentTime - lastFrameTime) + cyclesLeftOver);
		
		if (updateIterations > MAX_ITERATIONS)
		{
			updateIterations = MAX_ITERATIONS;
		}
		
		while (updateIterations > ANIMATION_TIMER_INTERVAL)
		{
			updateIterations -= ANIMATION_TIMER_INTERVAL;
			
			syncNetworkState(renderer->window, (float)ANIMATION_TIMER_INTERVAL);
			
			if (gGameState == GAME_STATE_ON)
			{
				animate(renderer->window, ANIMATION_TIMER_INTERVAL);
			}
			
			if (gGameShouldReset)
			{
				endGame();
				initGame();
			}
		}
		
		cyclesLeftOver = updateIterations;
		lastFrameTime = currentTime;
		
		if (needsToDrawScene)
		{
			renderFrame(renderer, drawScene);
		}
		
#ifndef _PROFILING
		SDL_bool hasAppFocus = (SDL_GetWindowFlags(renderer->window) & SDL_WINDOW_INPUT_FOCUS) != 0;
		// Restrict game to 30 fps when the fps flag is enabled as well as when we don't have app focus
		// This will allow the game to use less processing power when it's in the background,
		// which fixes a bug on macOS where the game can have huge CPU spikes when the window is completly obscured
		if (gFpsFlag || !hasAppFocus)
#else
		if (gFpsFlag)
#endif
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

		// deal with what music to play
		if (gAudioMusicFlag)
		{
			if (gGameState == GAME_STATE_OFF && !isPlayingMainMenuMusic())
			{
				stopMusic();
#ifndef _PROFILING
				if (hasAppFocus)
#endif
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

static void removeJoyStick(SDL_Joystick **joysticks, SDL_JoystickID instanceID)
{
	for (int joyIndex = 0; joyIndex < MAX_SUPPORTED_JOYSTICKS; joyIndex++)
	{
		if (joysticks[joyIndex] != NULL)
		{
			if (!SDL_JoystickGetAttached(joysticks[joyIndex]))
			{
				joysticks[joyIndex] = NULL;
			}
			else if (SDL_JoystickInstanceID(joysticks[joyIndex]) == instanceID)
			{
				SDL_JoystickClose(joysticks[joyIndex]);
				joysticks[joyIndex] = NULL;
			}
		}
	}
}

static void addJoyStick(SDL_Joystick **joysticks, int deviceIndex)
{
	SDL_Joystick *newJoystick = SDL_JoystickOpen(deviceIndex);
	if (newJoystick == NULL)
	{
		return;
	}
	
	for (int joyIndex = 0; joyIndex < MAX_SUPPORTED_JOYSTICKS; joyIndex++)
	{
		if (joysticks[joyIndex] != NULL && SDL_JoystickGetAttached(joysticks[joyIndex]) && SDL_JoystickInstanceID(joysticks[joyIndex]) == SDL_JoystickInstanceID(newJoystick))
		{
			return;
		}
	}
	
	for (int joyIndex = 0; joyIndex < MAX_SUPPORTED_JOYSTICKS; joyIndex++)
	{
		if (joysticks[joyIndex] == NULL || !SDL_JoystickGetAttached(joysticks[joyIndex]))
		{
			joysticks[joyIndex] = newJoystick;
			return;
		}
	}
	
	SDL_JoystickClose(newJoystick);
}

static void reloadJoyStickForInput(SDL_Joystick **joysticks, Input *input)
{
	setJoystickIdFromGuid(joysticks, &input->joy_right_id, input->joy_right_guid);
	setJoystickIdFromGuid(joysticks, &input->joy_left_id, input->joy_left_guid);
	setJoystickIdFromGuid(joysticks, &input->joy_up_id, input->joy_up_guid);
	setJoystickIdFromGuid(joysticks, &input->joy_down_id, input->joy_down_guid);
	setJoystickIdFromGuid(joysticks, &input->joy_weap_id, input->joy_weap_guid);
}

static void reloadJoySticks(SDL_Joystick **joysticks)
{
	reloadJoyStickForInput(joysticks, &gRedRoverInput);
	reloadJoyStickForInput(joysticks, &gPinkBubbleGumInput);
	reloadJoyStickForInput(joysticks, &gGreenTreeInput);
	reloadJoyStickForInput(joysticks, &gBlueLightningInput);
}

static void initJoySticks(SDL_Joystick **joysticks)
{
	int numJoySticks = SDL_NumJoysticks();
	if (numJoySticks > 0)
	{
		if (numJoySticks > MAX_SUPPORTED_JOYSTICKS)
		{
			numJoySticks = MAX_SUPPORTED_JOYSTICKS;
		}
		
		int numJoysticksCreated = 0;
		for (int joyIndex = 0; joyIndex < numJoySticks; joyIndex++)
		{
			SDL_Joystick *joystick = SDL_JoystickOpen(joyIndex);
			if (joystick != NULL)
			{
				joysticks[numJoysticksCreated] = joystick;
				numJoysticksCreated++;
			}
		}
	}
}

int main(int argc, char *argv[])
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO) < 0)
	{
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		return -1;
	}
	
#ifdef MAC_OS_X
	// The current working directory should point to our base path, particularly on macOS
	const char *baseDirectory = SDL_GetBasePath();
	if (baseDirectory != NULL && chdir(baseDirectory) != 0)
	{
		fprintf(stderr, "Failed to change current working directory to %s\n", baseDirectory);
		return -4;
	}
#endif

	SDL_Joystick *joysticks[MAX_SUPPORTED_JOYSTICKS] = {0};
	initJoySticks(joysticks);

	if (!initAudio())
	{
		return -3;
	};

	readDefaults(joysticks);

	// Create renderer
#ifdef _PROFILING
	SDL_bool vsync = SDL_FALSE;
#else
	SDL_bool vsync = gVsyncFlag;
#endif
	
	Renderer renderer;
	createRenderer(&renderer, gWindowWidth, gWindowHeight, gFullscreenFlag, vsync, gFsaaFlag);
	
	if (!initFont(&renderer))
	{
		return -2;
	}
	
	// Initialize game related things
	// init random number generator
	mt_init();
	
#ifdef _PROFILING
	gDrawFPS = SDL_TRUE;
#endif
	
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
	
	// Create netcode buffers and mutex's in case we need them later
	initializeNetworkBuffers();

	// Start the game event loop
	eventLoop(&renderer, joysticks);
	
	// Prepare to quit
	
	// Save user defaults
	writeDefaults(&renderer);
	
	if (gNetworkConnection)
	{
		if (gNetworkConnection->type == NETWORK_SERVER_TYPE)
		{
			GameMessage message;
			message.type = QUIT_MESSAGE_TYPE;
			sendToClients(0, &message);
		}
		else if (gNetworkConnection->type == NETWORK_CLIENT_TYPE && gNetworkConnection->character)
		{
			GameMessage message;
			message.type = QUIT_MESSAGE_TYPE;
			sendToServer(message);
		}
		
		// Wait for the thread to finish before we terminate the main thread
		while (gNetworkConnection != NULL)
		{
			syncNetworkState(renderer.window, (float)ANIMATION_TIMER_INTERVAL);
			SDL_Delay(10);
		}
	}

	SDL_Quit();
    return 0;
}
