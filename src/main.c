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

#include "sdl.h"
#include "platforms.h"
#include "characters.h"
#include "scenery.h"
#include "input.h"
#include "console.h"
#include "animation.h"
#include "weapon.h"
#include "text.h"
#include "audio.h"
#include "game_menus.h"
#include "network.h"
#include "mt_random.h"
#include "quit.h"
#include "time.h"
#include "gamepad.h"
#include "globals.h"
#include "window.h"

#define MATH_3D_IMPLEMENTATION
#include "math_3d.h"

#ifdef MAC_OS_X
#include "osx.h"
#endif

#ifdef WINDOWS
#include "linux.h"
#endif

#ifdef linux
#include "linux.h"
#endif

bool gGameHasStarted;
bool gGameShouldReset;
int32_t gGameStartNumber;
int gGameWinner;

GamepadManager *gGamepadManager;
static GamepadIndex gGamepads[4] = {INVALID_GAMEPAD_INDEX, INVALID_GAMEPAD_INDEX, INVALID_GAMEPAD_INDEX, INVALID_GAMEPAD_INDEX};

// Console flag indicating if we can use the console
bool gConsoleFlag = false;

// audio flags
bool gAudioEffectsFlag = true;
bool gAudioMusicFlag = true;

// video flags
static bool gFpsFlag = false;
static bool gFullscreenFlag = false;
static bool gVsyncFlag = true;
static bool gFsaaFlag = true;
static int32_t gWindowWidth = 800;
static int32_t gWindowHeight = 500;

bool gValidDefaults = false;

// Lives
int gCharacterLives =							5;
int gCharacterNetLives =						5;

// Fullscreen video state.
static bool gConsoleActivated =				false;
bool gDrawFPS =								false;
bool gDrawPings = false;

static GameState gGameState;

static uint32_t gEscapeHeldDownTimer;

typedef struct
{
	Renderer *renderer;
	bool *needsToDrawScene;
} WindowEventContext;

#define MAX_CHARACTER_LIVES 10
#define CHARACTER_ICON_DISPLACEMENT 5.0f
#define CHARACTER_ICON_OFFSET -8.5f

// in seconds
#define FIRST_NEW_GAME_COUNTDOWN 5
#define LATER_NEW_GAME_COUNTDOWN 3

static void initScene(Renderer *renderer);

static void readDefaults(void);
static void writeDefaults(Renderer *renderer);

static void resetGame(void);

static void drawBlackBox(Renderer *renderer)
{
	static BufferArrayObject vertexArrayObject;
	static BufferObject indicesBufferObject;
	static bool initializedBuffers;
	
	if (!initializedBuffers)
	{
		const float vertices[] =
		{
			-9.5f, 9.5f, 0.0f,
			9.5f, 9.5f, 0.0f,
			9.5f, -9.5f, 0.0f,
			-9.5f, -9.5f, 0.0f
		};
		
		vertexArrayObject = createVertexArrayObject(renderer, vertices, sizeof(vertices));
		indicesBufferObject = rectangleIndexBufferObject(renderer);
		
		initializedBuffers = true;
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

	gRedRoverInput.character = &gRedRover;
	gGreenTreeInput.character = &gGreenTree;
	gPinkBubbleGumInput.character = &gPinkBubbleGum;
	gBlueLightningInput.character = &gBlueLightning;

	initConsole();

	gGameState = GAME_STATE_OFF;

	initMenus();
}

#define MAX_EXPECTED_SCAN_LENGTH 512
static bool scanExpectedString(FILE *fp, const char *expectedString)
{
	size_t expectedStringLength = strlen(expectedString);
	if (expectedStringLength > MAX_EXPECTED_SCAN_LENGTH - 1)
	{
		return false;
	}
	
	char buffer[MAX_EXPECTED_SCAN_LENGTH] = {0};
	if (fread(buffer, expectedStringLength, 1, fp) < 1)
	{
		return false;
	}
	
	return (strcmp(buffer, expectedString) == 0);
}

static bool scanLineTerminatingString(FILE *fp, char *destBuffer, size_t maxBufferSize)
{
	char *safeBuffer = calloc(maxBufferSize, 1);
	if (safeBuffer == NULL) return false;
	
	size_t safeBufferIndex = 0;
	while (safeBufferIndex < maxBufferSize - 1 && ferror(fp) == 0)
	{
		int byte = fgetc(fp);
		if (byte == EOF || byte == '\n') break;
		
		safeBuffer[safeBufferIndex++] = (char)byte;
	}
	
	bool success;
	if (ferror(fp) != 0 || feof(fp) != 0 || safeBufferIndex >= maxBufferSize - 1)
	{
		success = false;
	}
	else
	{
		memcpy(destBuffer, safeBuffer, maxBufferSize);
		success = true;
	}
	
	free(safeBuffer);
	
	return success;
}

static bool readCharacterInputDefaults(FILE *fp, const char *characterName, Input *input, int defaultsVersion)
{
	bool readInputDefaults = false;
	
	if (!scanExpectedString(fp, characterName)) goto read_input_cleanup;
	
	int right = 0;
	if (fscanf(fp, " key right: %d\n", &right) < 1) goto read_input_cleanup;
	
	if (!scanExpectedString(fp, characterName)) goto read_input_cleanup;
	
	int left = 0;
	if (fscanf(fp, " key left: %d\n", &left) < 1) goto read_input_cleanup;
	
	if (!scanExpectedString(fp, characterName)) goto read_input_cleanup;
	
	int up = 0;
	if (fscanf(fp, " key up: %d\n", &up) < 1) goto read_input_cleanup;
	
	if (!scanExpectedString(fp, characterName)) goto read_input_cleanup;
	
	int down = 0;
	if (fscanf(fp, " key down: %d\n", &down) < 1) goto read_input_cleanup;
	
	if (!scanExpectedString(fp, characterName)) goto read_input_cleanup;
	
	int weapon = 0;
	if (fscanf(fp, " key weapon: %d\n", &weapon) < 1) goto read_input_cleanup;
	
	initInput(input, right, left, up, down, weapon);
	
	readInputDefaults = true;
	
read_input_cleanup:
	
	return readInputDefaults;
}

static void readDefaults(void)
{
	// this would be a good time to get the default user name
#ifdef MAC_OS_X
	getDefaultUserName(gUserNameString, MAX_USER_NAME_SIZE - 1);
	gUserNameStringIndex = (int)strlen(gUserNameString);
#else
	char *randomNames[] = { "Tale", "Backer", "Hop", "Expel", "Rida", "Tao", "Eyez", "Phia", "Sync", "Witty", "Poet", "Roost", "Kuro", "Spot", "Carb", "Unow", "Gil", "Needle", "Oxy", "Kale" };
	
	int randomNameIndex = (int)(mt_random() % (sizeof(randomNames) / sizeof(randomNames[0])));
	char *randomName = randomNames[randomNameIndex];
	
	strncpy(gUserNameString, randomName, MAX_USER_NAME_SIZE - 1);
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
	
	// We don't handle reading joystick inputs anymore and need to reset defaults
	if (defaultsVersion < 4) goto cleanup;

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
		gVsyncFlag = true;
	}
	
	int fpsFlag = 0;
	if (fscanf(fp, "fps flag: %i\n", &fpsFlag) < 1) goto cleanup;
	
	int aaFlag = 0;
	if (fscanf(fp, "FSAA flag: %i\n", &aaFlag) < 1) goto cleanup;
	
	if (defaultsVersion < 2)
	{
		gFsaaFlag = true;
	}
	else
	{
		gFsaaFlag = (aaFlag != 0);
	}

	int fullscreenFlag = 0;
	if (fscanf(fp, "Fullscreen flag: %d\n", &fullscreenFlag) < 1) goto cleanup;
	if (defaultsVersion < 2)
	{
		gFullscreenFlag = false;
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
	
	if (!readCharacterInputDefaults(fp, "Pink Bubblegum", &gPinkBubbleGumInput, defaultsVersion)) goto cleanup;
	if (!readCharacterInputDefaults(fp, "Red Rover", &gRedRoverInput, defaultsVersion)) goto cleanup;
	if (!readCharacterInputDefaults(fp, "Green Tree", &gGreenTreeInput, defaultsVersion)) goto cleanup;
	if (!readCharacterInputDefaults(fp, "Blue Lightning", &gBlueLightningInput, defaultsVersion)) goto cleanup;
	
	if (!scanExpectedString(fp, "Server IP Address: ")) goto cleanup;
	if (!scanLineTerminatingString(fp, gServerAddressString, sizeof(gServerAddressString))) goto cleanup;
	
	gServerAddressStringIndex = (int)strlen(gServerAddressString);
	
	if (!scanExpectedString(fp, "\nNet name: ")) goto cleanup;
	if (!scanLineTerminatingString(fp, gUserNameString, sizeof(gUserNameString))) goto cleanup;
	
	gUserNameStringIndex = (int)strlen(gUserNameString);
	
	int audioEffectsFlag = 0;
	if (fscanf(fp, "\nAudio effects: %i\n", &audioEffectsFlag) < 1) goto cleanup;
	gAudioEffectsFlag = (audioEffectsFlag != 0);
	
	int audioMusicFlag = 0;
	if (fscanf(fp, "Music: %i\n", &audioMusicFlag) < 1) goto cleanup;
	gAudioMusicFlag = (audioMusicFlag != 0);

	gValidDefaults = true;
cleanup:
#ifdef _DEBUG
	if (!gValidDefaults)
	{
		fprintf(stderr, "Warning: user defaults read were not valid\n");
	}
#endif
	fclose(fp);
}

static void writeCharacterInput(FILE *fp, const char *characterName, Input *input)
{
	fprintf(fp, "%s key right: %i\n", characterName, input->r_id);
	fprintf(fp, "%s key left: %i\n", characterName, input->l_id);
	fprintf(fp, "%s key up: %i\n", characterName, input->u_id);
	fprintf(fp, "%s key down: %i\n", characterName, input->d_id);
	fprintf(fp, "%s key weapon: %i\n", characterName, input->weap_id);
	
	fprintf(fp, "\n");
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
	fprintf(fp, "\nDefaults version: 4\n");

	fclose(fp);
}

void initGame(ZGWindow *window, bool firstGame)
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
	
	if (firstGame)
	{
		if (gNetworkConnection != NULL)
		{
			gPinkBubbleGumInput.gamepadIndex = gGamepads[0];
			gRedRoverInput.gamepadIndex = gGamepads[1];
			gGreenTreeInput.gamepadIndex = gGamepads[2];
			gBlueLightningInput.gamepadIndex = gGamepads[3];
		}
		else
		{
			// Automatically assign gamepads to the last human character slots
			// We don't assign gamepads to the first human character slots because we want the first slots to be reserved for keyboard usage
			// Pink bubblegum (first character) in particular has ideal default keyboard controls
			Input *characterInputs[4] = {&gPinkBubbleGumInput, &gRedRoverInput, &gGreenTreeInput, &gBlueLightningInput};
			int16_t numberCharacters = (int16_t)(sizeof(characterInputs) / sizeof(*characterInputs));
			int16_t maxGamepads = (int16_t)(sizeof(gGamepads) / sizeof(*gGamepads));

			for (int16_t characterIndex = 0; characterIndex < numberCharacters; characterIndex++)
			{
				if (characterInputs[characterIndex]->character->state == CHARACTER_AI_STATE)
				{
					int16_t gamepadCount = 0;
					for (int16_t gamepadIndex = 0; gamepadIndex < maxGamepads; gamepadIndex++)
					{
						if (gGamepads[gamepadIndex] != INVALID_GAMEPAD_INDEX)
						{
							gamepadCount++;
						}
					}
					
					int16_t characterStartIndex = characterIndex >= gamepadCount ? characterIndex - gamepadCount : 0;
					int16_t gamepadMappingIndex = 0;
					
					for (int16_t characterMappingIndex = characterStartIndex; characterMappingIndex < numberCharacters; characterMappingIndex++)
					{
						if (characterInputs[characterMappingIndex]->character->state == CHARACTER_AI_STATE)
						{
							break;
						}
						
						bool availableGamepad = true;
						while (gGamepads[gamepadMappingIndex] == INVALID_GAMEPAD_INDEX)
						{
							gamepadMappingIndex++;
							if (gamepadMappingIndex >= maxGamepads)
							{
								availableGamepad = false;
								break;
							}
						}
						
						if (!availableGamepad)
						{
							break;
						}
						
						characterInputs[characterMappingIndex]->gamepadIndex = gGamepads[gamepadMappingIndex];
						
						const char *controllerName = gamepadName(gGamepadManager, gGamepads[gamepadMappingIndex]);
						if (controllerName != NULL)
						{
							strncpy(characterInputs[characterMappingIndex]->character->controllerName, controllerName, MAX_CONTROLLER_NAME_SIZE - 1);
						}
						
						gamepadMappingIndex++;
						if (gamepadMappingIndex >= maxGamepads)
						{
							break;
						}
					}
					
					break;
				}
			}
		}
	}

	gRedRover.lives = initialNumberOfLives;
	gGreenTree.lives = initialNumberOfLives;
	gPinkBubbleGum.lives = initialNumberOfLives;
	gBlueLightning.lives = initialNumberOfLives;

	gGameState = GAME_STATE_ON;

	// wait until the game can be started.
	gGameStartNumber = firstGame ? FIRST_NEW_GAME_COUNTDOWN : LATER_NEW_GAME_COUNTDOWN;
	
	gEscapeHeldDownTimer = 0;
	
	if (firstGame && gAudioMusicFlag)
	{
		bool windowFocus = ZGWindowHasFocus(window);
		playGameMusic(!windowFocus);
	}
}

void endGame(ZGWindow *window, bool lastGame)
{
	gGameHasStarted = false;

	endAnimation();

	gGameState = GAME_STATE_OFF;

	gGameWinner = NO_CHARACTER;
	gGameShouldReset = false;
	
	if (lastGame)
	{
		gPinkBubbleGumInput.gamepadIndex = INVALID_GAMEPAD_INDEX;
		gRedRoverInput.gamepadIndex = INVALID_GAMEPAD_INDEX;
		gGreenTreeInput.gamepadIndex = INVALID_GAMEPAD_INDEX;
		gBlueLightningInput.gamepadIndex = INVALID_GAMEPAD_INDEX;
		
		memset(gPinkBubbleGum.controllerName, 0, MAX_CONTROLLER_NAME_SIZE);
		memset(gRedRover.controllerName, 0, MAX_CONTROLLER_NAME_SIZE);
		memset(gGreenTree.controllerName, 0, MAX_CONTROLLER_NAME_SIZE);
		memset(gBlueLightning.controllerName, 0, MAX_CONTROLLER_NAME_SIZE);
		
		if (gAudioMusicFlag)
		{
			bool windowFocus = ZGWindowHasFocus(window);
			playMainMenuMusic(!windowFocus);
		}
	}
}

static void exitGame(ZGWindow *window)
{
	resetCharacterWins();
	endGame(window, true);
	
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
		last_frame_time = ZGGetTicks() / 1000.0;
        last_fps_time = last_frame_time;
    }

	now = ZGGetTicks() / 1000.0;
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
		double currentTime = ZGGetTicks() / 1000.0;
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
				
				size_t length = strlen(pingString);
				if (length > 0)
				{
					drawString(renderer, modelViewMatrix, (color4_t){character->red, character->green, character->blue, 1.0f}, 0.16f * length, 0.5f, pingString);
				}
			}
		}
		else if (gNetworkConnection->type == NETWORK_SERVER_TYPE)
		{
			static char pingStrings[3][256] = {{0}, {0}, {0}};
			
			bool rebuildStrings = (lastPingDisplayTime < 0.0 || currentTime - lastPingDisplayTime >= 1.0);
			
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
					
					size_t length = strlen(pingStrings[pingAddressIndex]);
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
		drawAllCharacterInfo(renderer, characterIconTranslations, gGameHasStarted);
		
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
					gGameHasStarted = true;
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
					drawStringScaled(renderer, modelViewMatrix, (color4_t){0.0f, 0.0f, 0.4f, 1.0f}, 0.004f, "Fire to play again or Escape to quit");
				}
				else if (gNetworkConnection != NULL && gNetworkConnection->type == NETWORK_CLIENT_TYPE && gEscapeHeldDownTimer > 0)
				{
					drawStringScaled(renderer, modelViewMatrix, (color4_t){0.0f, 0.0f, 0.4f, 1.0f}, 0.004f, "Hold Escape to quit...");
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
		drawStringScaled(renderer, gameTitleModelViewMatrix, (color4_t){0.3f, 0.2f, 1.0f, 0.7f}, 0.00592f, "Sky Checkers");
		
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

static void handleKeyDownEvent(SDL_Event *event, Renderer *renderer)
{
	ZGWindow *window = renderer->window;
#ifdef MAC_OS_X
	SDL_Keymod metaMod = (KMOD_LGUI | KMOD_RGUI);
#else
	SDL_Keymod metaMod = (KMOD_LCTRL | KMOD_RCTRL);
#endif
	
	SDL_Scancode scancode = event->key.keysym.scancode;
	uint16_t mod = event->key.keysym.mod;
	
	if (gMenuPendingOnKeyCode)
	{
		setPendingKeyCode(scancode);
	}
	else if (scancode == SDL_SCANCODE_V && (mod & metaMod) != 0 && SDL_HasClipboardText())
	{
		char *clipboardText = SDL_GetClipboardText();
		if (clipboardText != NULL)
		{
			writeTextInput(clipboardText, 128);
		}
	}
#ifdef linux
	else if (scancode == SDL_SCANCODE_RETURN && (mod & KMOD_ALT) != 0)
	{
		const char *fullscreenErrorString = NULL;
		if (!ZGWindowIsFullscreen(renderer->window))
		{
			if (!ZGSetWindowFullscreen(renderer->window, true, &fullscreenErrorString))
			{
				fprintf(stderr, "Failed to set fullscreen because: %s\n", fullscreenErrorString);
			}
			else
			{
				renderer->fullscreen = true;
			}
		}
		else
		{
			if (!ZGSetWindowFullscreen(renderer->window, false, &fullscreenErrorString))
			{
				fprintf(stderr, "Failed to escape fullscreen because: %s\n", fullscreenErrorString);
			}
			else
			{
				renderer->fullscreen = false;
			}
		}
	}
#endif
	else if (!gConsoleActivated && gGameState == GAME_STATE_ON && gGameWinner != NO_CHARACTER &&
		(scancode == gPinkBubbleGumInput.weap_id || scancode == gRedRoverInput.weap_id ||
		 scancode == gBlueLightningInput.weap_id || scancode == gGreenTreeInput.weap_id ||
		scancode == SDL_SCANCODE_RETURN || scancode == SDL_SCANCODE_KP_ENTER))
	{
		// new game
		if (!gNetworkConnection || gNetworkConnection->type == NETWORK_SERVER_TYPE)
		{
			resetGame();
		}
	}
	else if (scancode == SDL_SCANCODE_RETURN)
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
			GameMenuContext menuContext;
			menuContext.gameState = &gGameState;
			menuContext.window = window;
			
			invokeMenu(&menuContext);
		}

		else if (gConsoleActivated)
		{
			executeConsoleCommand();
		}
	}

	else if (scancode == SDL_SCANCODE_DOWN)
	{
		if (gNetworkAddressFieldIsActive)
			return;

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

	else if (scancode == SDL_SCANCODE_UP)
	{
		if (gNetworkAddressFieldIsActive)
			return;

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

	else if (scancode == SDL_SCANCODE_ESCAPE)
	{
		if (gGameState == GAME_STATE_OFF)
		{
			if (gEscapeHeldDownTimer == 0)
			{
				if (gNetworkAddressFieldIsActive)
				{
					gNetworkAddressFieldIsActive = false;
				}
				else if (gNetworkUserNameFieldIsActive)
				{
					gNetworkUserNameFieldIsActive = false;
				}
				else if (gDrawArrowsForCharacterLivesFlag)
				{
					gDrawArrowsForCharacterLivesFlag = false;
				}
				else if (gDrawArrowsForNetPlayerLivesFlag)
				{
					gDrawArrowsForNetPlayerLivesFlag = false;
				}
				else if (gDrawArrowsForAIModeFlag)
				{
					gDrawArrowsForAIModeFlag = false;
				}
				else if (gDrawArrowsForNumberOfNetHumansFlag)
				{
					gDrawArrowsForNumberOfNetHumansFlag = false;
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
				exitGame(window);
			}
			else if (gEscapeHeldDownTimer == 0)
			{
				gEscapeHeldDownTimer = ZGGetTicks();
			}
		}
	}

	else if (scancode == SDL_SCANCODE_GRAVE && gGameState == GAME_STATE_ON)
	{
		if (gConsoleFlag)
		{
			if (gConsoleActivated)
			{
				gConsoleActivated = false;
			}
			else
			{
				gConsoleActivated = true;
			}
		}
	}

	else if (scancode == SDL_SCANCODE_BACKSPACE)
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
	
	if (!(scancode == SDL_SCANCODE_RETURN && (mod & metaMod) != 0) && gGameState == GAME_STATE_ON)
	{
		if (!gConsoleActivated)
		{
			performDownKeyAction(&gRedRoverInput, event);
			performDownKeyAction(&gGreenTreeInput, event);
			performDownKeyAction(&gPinkBubbleGumInput, event);
			performDownKeyAction(&gBlueLightningInput, event);
		}
	}
}

static void handleKeyUpEvent(SDL_Event *event)
{
	SDL_Scancode scancode = event->key.keysym.scancode;
	uint16_t mod = event->key.keysym.mod;
	
	if (scancode == SDL_SCANCODE_ESCAPE)
	{
		gEscapeHeldDownTimer = 0;
	}
	
#ifdef MAC_OS_X
	SDL_Keymod metaMod = (KMOD_LGUI | KMOD_RGUI);
#else
	SDL_Keymod metaMod = (KMOD_LCTRL | KMOD_RCTRL);
#endif
	
	if (!(scancode == SDL_SCANCODE_RETURN && (mod & metaMod) != 0) && gGameState == GAME_STATE_ON)
	{
		performUpKeyAction(&gRedRoverInput, event);
		performUpKeyAction(&gGreenTreeInput, event);
		performUpKeyAction(&gPinkBubbleGumInput, event);
		performUpKeyAction(&gBlueLightningInput, event);
	}
}

static void handleTextInputEvent(SDL_Event *event)
{
	writeTextInput(event->text.text, SDL_TEXTINPUTEVENT_TEXT_SIZE);
}

#define MAX_ITERATIONS (25 * ANIMATION_TIMER_INTERVAL)

static void resetGame(void)
{
	if (gNetworkConnection)
	{
		GameMessage message;
		message.type = GAME_RESET_MESSAGE_TYPE;
		sendToClients(0, &message);
	}
	gGameShouldReset = true;
}

static void pollGamepads(GamepadManager *gamepadManager, const void *systemEvent)
{
	uint16_t gamepadEventsCount = 0;
	GamepadEvent *gamepadEvents = pollGamepadEvents(gamepadManager, systemEvent, &gamepadEventsCount);
	for (uint16_t gamepadEventIndex = 0; gamepadEventIndex < gamepadEventsCount; gamepadEventIndex++)
	{
		GamepadEvent *gamepadEvent = &gamepadEvents[gamepadEventIndex];

		performGamepadAction(&gRedRoverInput, gamepadEvent);
		performGamepadAction(&gGreenTreeInput, gamepadEvent);
		performGamepadAction(&gPinkBubbleGumInput, gamepadEvent);
		performGamepadAction(&gBlueLightningInput, gamepadEvent);
	}
	
	if (!gConsoleActivated && gGameState == GAME_STATE_ON && gGameWinner != NO_CHARACTER && (gNetworkConnection == NULL || gNetworkConnection->type == NETWORK_SERVER_TYPE) && (gRedRoverInput.weap || gGreenTreeInput.weap || gPinkBubbleGumInput.weap || gBlueLightningInput.weap))
	{
		resetGame();
	}
}

static void handleWindowEvent(ZGWindowEvent event, void *context)
{
	WindowEventContext *windowEventContext = context;
	switch (event.type)
	{
		case ZGWindowEventTypeResize:
			updateViewport(windowEventContext->renderer, event.width, event.height);
			break;
		case ZGWindowEventTypeFocusGained:
			unPauseMusic();
			break;
		case ZGWindowEventTypeFocusLost:
			pauseMusic();
			break;
		case ZGWindowEventTypeShown:
			*windowEventContext->needsToDrawScene = true;
			break;
		case ZGWindowEventTypeHidden:
			*windowEventContext->needsToDrawScene = false;
			break;
	}
}

static void eventLoop(Renderer *renderer, GamepadManager *gamepadManager)
{
	SDL_Event event;
	bool needsToDrawScene = true;
	bool done = false;

	int fps = 30;
	int delay = 1000 / fps;
	int thenTicks = -1;
	int nowTicks;
	
	WindowEventContext windowEventContext;
	windowEventContext.renderer = renderer;
	windowEventContext.needsToDrawScene = &needsToDrawScene;
	
	ZGSetWindowEventHandler(renderer->window, &windowEventContext, handleWindowEvent);

	while (!done)
	{
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_KEYDOWN:
					handleKeyDownEvent(&event, renderer);
					break;
				case SDL_KEYUP:
					handleKeyUpEvent(&event);
					break;
				case SDL_TEXTINPUT:
					handleTextInputEvent(&event);
					break;
				case SDL_QUIT:
					done = true;
					break;
			}
#ifndef MAC_OS_X
			pollGamepads(gamepadManager, &event);
			ZGPollWindowEvents(renderer->window, &event);
#endif
		}
		
		pollGamepads(gamepadManager, NULL);
		
		// Update game state
		// http://ludobloom.com/tutorials/timestep.html
		static double lastFrameTime = 0.0;
		static double cyclesLeftOver = 0.0;
		
		double currentTime = ZGGetTicks() / 1000.0;
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
			
			if (gGameState == GAME_STATE_ON && gEscapeHeldDownTimer > 0 && ZGGetTicks() - gEscapeHeldDownTimer > 700)
			{
				exitGame(renderer->window);
			}
			
			if (gGameShouldReset)
			{
				endGame(renderer->window, false);
				initGame(renderer->window, false);
			}
		}
		
		cyclesLeftOver = updateIterations;
		lastFrameTime = currentTime;
		
		if (needsToDrawScene)
		{
			renderFrame(renderer, drawScene);
		}
		
#ifndef _PROFILING
		bool hasAppFocus = ZGWindowHasFocus(renderer->window);
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
				nowTicks = ZGGetTicks();
				delay += (1000 / fps - (nowTicks - thenTicks));
				thenTicks = nowTicks;

				if (delay < 0)
					delay = 1000 / fps;
			}
			else
			{
				thenTicks = ZGGetTicks();
			}

			ZGDelay(delay);
		}
	}
}

static void gamepadAdded(GamepadIndex gamepadIndex)
{
	for (uint16_t gamepadIndex = 0; gamepadIndex < sizeof(gGamepads) / sizeof(*gGamepads); gamepadIndex++)
	{
		if (gGamepads[gamepadIndex] == INVALID_GAMEPAD_INDEX)
		{
			gGamepads[gamepadIndex] = gamepadIndex;
			break;
		}
	}
}

static void gamepadRemoved(GamepadIndex gamepadIndex)
{
	for (uint16_t gamepadIndex = 0; gamepadIndex < sizeof(gGamepads) / sizeof(*gGamepads); gamepadIndex++)
	{
		if (gGamepads[gamepadIndex] == gamepadIndex)
		{
			gGamepads[gamepadIndex] = INVALID_GAMEPAD_INDEX;
			
			if (gPinkBubbleGumInput.gamepadIndex == gamepadIndex)
			{
				gPinkBubbleGumInput.gamepadIndex = INVALID_GAMEPAD_INDEX;
			}
			else if (gRedRoverInput.gamepadIndex == gamepadIndex)
			{
				gRedRoverInput.gamepadIndex = INVALID_GAMEPAD_INDEX;
			}
			else if (gGreenTreeInput.gamepadIndex == gamepadIndex)
			{
				gGreenTreeInput.gamepadIndex = INVALID_GAMEPAD_INDEX;
			}
			else if (gBlueLightningInput.gamepadIndex == gamepadIndex)
			{
				gBlueLightningInput.gamepadIndex = INVALID_GAMEPAD_INDEX;
			}
			
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		ZGQuit();
	}
	
#ifdef MAC_OS_X
	setUpCurrentWorkingDirectory();
#endif

	initAudio();
	
	// init random number generator
	mt_init();

	readDefaults();

	// Create renderer
#ifdef _PROFILING
	bool vsync = false;
#else
	bool vsync = gVsyncFlag;
#endif
	
	Renderer renderer;
	createRenderer(&renderer, gWindowWidth, gWindowHeight, gFullscreenFlag, vsync, gFsaaFlag);
	
	initText(&renderer);
	
	// Initialize game related things
	
#ifdef _PROFILING
	gDrawFPS = true;
#endif
	
	initScene(&renderer);
	
	gGamepadManager = initGamepadManager("Data/gamecontrollerdb.txt", gamepadAdded, gamepadRemoved);
	
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
	
	if (gAudioMusicFlag)
	{
		bool windowFocus = ZGWindowHasFocus(renderer.window);
		playMainMenuMusic(!windowFocus);
	}

	// Start the game event loop
	eventLoop(&renderer, gGamepadManager);
	
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
			ZGDelay(10);
		}
	}

	ZGQuit();
    return 0;
}
