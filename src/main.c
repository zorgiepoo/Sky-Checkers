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

#include "app.h"
#include "platforms.h"
#include "characters.h"
#include "scenery.h"
#include "input.h"
#include "animation.h"
#include "weapon.h"
#include "text.h"
#include "audio.h"
#include "menus.h"
#include "network.h"
#include "mt_random.h"
#include "quit.h"
#include "time.h"
#include "gamepad.h"
#include "globals.h"
#include "window.h"
#include "defaults.h"
#include "renderer_projection.h"

#if !PLATFORM_IOS
#include "console.h"
#endif

#include <string.h>
#include <stdlib.h>

#define MATH_3D_IMPLEMENTATION
#include "math_3d.h"

bool gGameHasStarted;
bool gGameShouldReset;
int32_t gGameStartNumber;
uint8_t gTutorialStage;
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

// online fields
#if PLATFORM_IOS
char gServerAddressString[MAX_SERVER_ADDRESS_SIZE] =  {0};
int gServerAddressStringIndex = 0;
#else
char gServerAddressString[MAX_SERVER_ADDRESS_SIZE] = "localhost";
int gServerAddressStringIndex = 9;
#endif

char gUserNameString[MAX_USER_NAME_SIZE];
int gUserNameStringIndex = 0;

bool gValidDefaults = false;

// Lives
int gCharacterLives = 5;

#if !PLATFORM_IOS
static bool gConsoleActivated;
#endif

bool gDrawFPS;
bool gDrawPings;

static GameState gGameState;

typedef struct
{
	Renderer renderer;
	bool needsToDrawScene;
	
	double lastFrameTime;
	double cyclesLeftOver;
	uint32_t lastRunloopTime;
} AppContext;

#define MAX_FPS_RATE 120

#define CHARACTER_ICON_DISPLACEMENT 5.0f
#define CHARACTER_ICON_OFFSET -8.5f

// in seconds
#define FIRST_NEW_GAME_COUNTDOWN 5
#define LATER_NEW_GAME_COUNTDOWN 3

static void initScene(Renderer *renderer);

static void readDefaults(void);
static void writeDefaults(Renderer *renderer);

static void exitGame(ZGWindow *window);
static void resetGame(void);

static void drawBlackBox(Renderer *renderer)
{
	static BufferArrayObject vertexArrayObject;
	static BufferObject indicesBufferObject;
	static bool initializedBuffers;
	
	if (!initializedBuffers)
	{
		const ZGFloat vertices[] =
		{
			-9.5f, 9.5f, 0.0f, 1.0,
			9.5f, 9.5f, 0.0f, 1.0,
			9.5f, -9.5f, 0.0f, 1.0,
			-9.5f, -9.5f, 0.0f, 1.0
		};
		
		vertexArrayObject = createVertexArrayObject(renderer, vertices, sizeof(vertices));
		indicesBufferObject = rectangleIndexBufferObject(renderer);
		
		initializedBuffers = true;
	}
	
	mat4_t modelViewMatrix = m4_mul(m4_translation((vec3_t){0.0f, 0.0f, -22.0f}), m4_scaling((vec3_t){computeProjectionAspectRatio(renderer), 1.0f, 1.0f}));
	
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
		initInput(&gRedRoverInput);
		initInput(&gGreenTreeInput);
		initInput(&gBlueLightningInput);
		initInput(&gPinkBubbleGumInput);

#if !PLATFORM_IOS
		setInputKeys(&gRedRoverInput, ZG_KEYCODE_B, ZG_KEYCODE_C, ZG_KEYCODE_F, ZG_KEYCODE_V, ZG_KEYCODE_G);
		setInputKeys(&gGreenTreeInput, ZG_KEYCODE_L, ZG_KEYCODE_J, ZG_KEYCODE_I, ZG_KEYCODE_K, ZG_KEYCODE_M);
		setInputKeys(&gBlueLightningInput, ZG_KEYCODE_D, ZG_KEYCODE_A, ZG_KEYCODE_W, ZG_KEYCODE_S, ZG_KEYCODE_Z);
		setInputKeys(&gPinkBubbleGumInput, ZG_KEYCODE_RIGHT, ZG_KEYCODE_LEFT, ZG_KEYCODE_UP, ZG_KEYCODE_DOWN, ZG_KEYCODE_SPACE);
#endif
		
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

#if !PLATFORM_IOS
	initConsole();
#endif

	gGameState = GAME_STATE_OFF;

	initMenus(renderer->window, &gGameState, exitGame);
#if PLATFORM_IOS
	showGameMenus(renderer->window);
#endif
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

#if !PLATFORM_IOS
static bool readCharacterKeyboardDefaults(FILE *fp, const char *characterName, Input *input, int defaultsVersion)
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
	
	initInput(input);
	setInputKeys(input, right, left, up, down, weapon);
	
	readInputDefaults = true;
	
read_input_cleanup:
	
	return readInputDefaults;
}
#endif

static void readDefaults(void)
{
	// this would be a good time to get the default user name
	getDefaultUserName(gUserNameString, MAX_USER_NAME_SIZE - 1);
	gUserNameStringIndex = (int)strlen(gUserNameString);
	if (gUserNameStringIndex == 0)
	{
		char *randomNames[] = { "Tale", "Backer", "Hop", "Expel", "Rida", "Tao", "Eyez", "Phia", "Sync", "Witty", "Poet", "Roost", "Kuro", "Spot", "Carb", "Unow", "Gil", "Needle", "Oxy", "Kale" };
		
		int randomNameIndex = (int)(mt_random() % (sizeof(randomNames) / sizeof(randomNames[0])));
		char *randomName = randomNames[randomNameIndex];
		
		strncpy(gUserNameString, randomName, MAX_USER_NAME_SIZE - 1);
		gUserNameStringIndex = (int)strlen(gUserNameString);
	}

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
	
	if (fscanf(fp, "Number of Net Humans: %i\n", &gNumberOfNetHumans) < 1) goto cleanup;
	if (gNumberOfNetHumans < 0 || gNumberOfNetHumans > 3)
	{
		gNumberOfNetHumans = 1;
	}
	
#if !PLATFORM_IOS
	if (!readCharacterKeyboardDefaults(fp, "Pink Bubblegum", &gPinkBubbleGumInput, defaultsVersion)) goto cleanup;
	if (!readCharacterKeyboardDefaults(fp, "Red Rover", &gRedRoverInput, defaultsVersion)) goto cleanup;
	if (!readCharacterKeyboardDefaults(fp, "Green Tree", &gGreenTreeInput, defaultsVersion)) goto cleanup;
	if (!readCharacterKeyboardDefaults(fp, "Blue Lightning", &gBlueLightningInput, defaultsVersion)) goto cleanup;
#endif
	
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

#if !PLATFORM_IOS
static void writeCharacterInput(FILE *fp, const char *characterName, Input *input)
{
	fprintf(fp, "%s key right: %i\n", characterName, input->r_id);
	fprintf(fp, "%s key left: %i\n", characterName, input->l_id);
	fprintf(fp, "%s key up: %i\n", characterName, input->u_id);
	fprintf(fp, "%s key down: %i\n", characterName, input->d_id);
	fprintf(fp, "%s key weapon: %i\n", characterName, input->weap_id);
	
	fprintf(fp, "\n");
}
#endif

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

	fprintf(fp, "\n");

	// character states
	fprintf(fp, "Pink Bubblegum state: %i\n", offlineCharacterState(&gPinkBubbleGum));
	fprintf(fp, "Red Rover state: %i\n", offlineCharacterState(&gRedRover));
	fprintf(fp, "Green Tree state: %i\n", offlineCharacterState(&gGreenTree));
	fprintf(fp, "Blue Lightning state: %i\n", offlineCharacterState(&gBlueLightning));

	fprintf(fp, "AI Mode: %i\n", gAIMode);
	fprintf(fp, "Number of Net Humans: %i\n", gNumberOfNetHumans);

	fprintf(fp, "\n");

	// Character defaults
	
#if !PLATFORM_IOS
	writeCharacterInput(fp, "Pink Bubblegum", &gPinkBubbleGumInput);
	fprintf(fp, "\n");
	
	writeCharacterInput(fp, "Red Rover", &gRedRoverInput);
	fprintf(fp, "\n");
	
	writeCharacterInput(fp, "Green Tree", &gGreenTreeInput);
	fprintf(fp, "\n");
	
	writeCharacterInput(fp, "Blue Lightning", &gBlueLightningInput);
	fprintf(fp, "\n");
#endif
	
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

void initGame(ZGWindow *window, bool firstGame, bool tutorial)
{
	loadTiles();

	loadCharacter(&gRedRover);
	loadCharacter(&gGreenTree);
	loadCharacter(&gPinkBubbleGum);
	loadCharacter(&gBlueLightning);
	
	if (tutorial)
	{
		Character *firstHumanCharacter;
		if (gPinkBubbleGum.state == CHARACTER_HUMAN_STATE)
		{
			firstHumanCharacter = &gPinkBubbleGum;
		}
		else if (gRedRover.state == CHARACTER_HUMAN_STATE)
		{
			firstHumanCharacter = &gRedRover;
		}
		else if (gGreenTree.state == CHARACTER_HUMAN_STATE)
		{
			firstHumanCharacter = &gGreenTree;
		}
		else
		{
			firstHumanCharacter = &gBlueLightning;
		}
		
		firstHumanCharacter->x = 2.0f * 2;
		firstHumanCharacter->y = 2.0f * 2;
		turnCharacter(firstHumanCharacter, UP);
	}

	startAnimation();
	
	int initialNumberOfLives;
	if (tutorial)
	{
		initialNumberOfLives = 1;
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
			setPlayerIndex(gGamepadManager, gPinkBubbleGumInput.gamepadIndex, 0);
			
			gRedRoverInput.gamepadIndex = gGamepads[1];
			setPlayerIndex(gGamepadManager, gRedRoverInput.gamepadIndex, 0);
			
			gGreenTreeInput.gamepadIndex = gGamepads[2];
			setPlayerIndex(gGamepadManager, gGreenTreeInput.gamepadIndex, 0);
			
			gBlueLightningInput.gamepadIndex = gGamepads[3];
			setPlayerIndex(gGamepadManager, gBlueLightningInput.gamepadIndex, 0);
		}
		else
		{
			// Automatically assign gamepads to the last human character slots
			// We don't assign gamepads to the first human character slots because we want the first slots to be reserved for keyboard usage, if there aren't enough gamepads for everyone
			// Pink bubblegum (first character) in particular has ideal default keyboard controls
			Input *characterInputs[4] = {&gPinkBubbleGumInput, &gRedRoverInput, &gGreenTreeInput, &gBlueLightningInput};
			
			uint8_t numberOfHumanPlayers = 0;
			for (uint8_t characterIndex = 0; characterIndex < sizeof(characterInputs) / sizeof(characterInputs[0]); characterIndex++)
			{
				if (characterInputs[characterIndex]->character->state == CHARACTER_HUMAN_STATE)
				{
					numberOfHumanPlayers++;
				}
			}
			
			uint16_t maxGamepads = (uint16_t)(sizeof(gGamepads) / sizeof(*gGamepads));
			uint16_t gamepadCount = 0;
			for (uint16_t gamepadIndex = 0; gamepadIndex < maxGamepads; gamepadIndex++)
			{
				if (gGamepads[gamepadIndex] != INVALID_GAMEPAD_INDEX)
				{
					gamepadCount++;
				}
			}
			
			if (gamepadCount > 0 && numberOfHumanPlayers > 0)
			{
				// 3 gamepads, 2 human players .. human players <= gamepads
				// assign humans gamepads based on humanCharacterSlotIndex = 0
				
				// 3 human players, 2 gamepads .. human players > gamepads
				// assign humans gamepads based on humanCharacterSlotIndex = 1 (or human players - gamepads available)
				
				uint8_t startingHumanCharacterSlot = numberOfHumanPlayers > gamepadCount ? numberOfHumanPlayers - gamepadCount : 0;
				uint8_t currentHumanCharacterSlot = 0;
				uint16_t currentGamepadIndex = 0;
				for (uint8_t characterIndex = 0; characterIndex < sizeof(characterInputs) / sizeof(characterInputs[0]); characterIndex++)
				{
					Input *characterInput = characterInputs[characterIndex];
					Character *character = characterInput->character;
					
					if (character->state == CHARACTER_HUMAN_STATE)
					{
						if (currentHumanCharacterSlot >= startingHumanCharacterSlot)
						{
							uint16_t gamepadIndex;
							for (gamepadIndex = currentGamepadIndex; gamepadIndex < maxGamepads; gamepadIndex++)
							{
								if (gGamepads[gamepadIndex] != INVALID_GAMEPAD_INDEX)
								{
									characterInput->gamepadIndex = gGamepads[gamepadIndex];
									setPlayerIndex(gGamepadManager, characterInput->gamepadIndex, characterIndex);
									
									const char *controllerName = gamepadName(gGamepadManager, characterInput->gamepadIndex);
									if (controllerName != NULL)
									{
										strncpy(character->controllerName, controllerName, MAX_CONTROLLER_NAME_SIZE - 1);
									}
									
									break;
								}
							}
							currentGamepadIndex = gamepadIndex + 1;
						}
						currentHumanCharacterSlot++;
					}
				}
			}
		}
	}

	gRedRover.lives = initialNumberOfLives;
	gGreenTree.lives = initialNumberOfLives;
	gPinkBubbleGum.lives = initialNumberOfLives;
	gBlueLightning.lives = initialNumberOfLives;

	gGameState = tutorial ? GAME_STATE_TUTORIAL : GAME_STATE_ON;

	// wait until the game can be started.
	if (tutorial)
	{
		gGameStartNumber = 0;
	}
	else if (firstGame)
	{
		gGameStartNumber = FIRST_NEW_GAME_COUNTDOWN;
	}
	else
	{
		gGameStartNumber = LATER_NEW_GAME_COUNTDOWN;
	}
	
	gTutorialStage = 0;
	
	if (firstGame && gAudioMusicFlag)
	{
		bool windowFocus = ZGWindowHasFocus(window);
		playGameMusic(!windowFocus);
	}
	
	if (firstGame)
	{
		ZGAppSetAllowsScreenSaver(false);
#if PLATFORM_IOS
		ZGInstallTouchGestures(window);
#endif
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
		setPlayerIndex(gGamepadManager, gPinkBubbleGumInput.gamepadIndex, UNSET_PLAYER_INDEX);
		gPinkBubbleGumInput.gamepadIndex = INVALID_GAMEPAD_INDEX;
		
		setPlayerIndex(gGamepadManager, gRedRoverInput.gamepadIndex, UNSET_PLAYER_INDEX);
		gRedRoverInput.gamepadIndex = INVALID_GAMEPAD_INDEX;
		
		setPlayerIndex(gGamepadManager, gGreenTreeInput.gamepadIndex, UNSET_PLAYER_INDEX);
		gGreenTreeInput.gamepadIndex = INVALID_GAMEPAD_INDEX;
		
		setPlayerIndex(gGamepadManager, gBlueLightningInput.gamepadIndex, UNSET_PLAYER_INDEX);
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
		
		ZGAppSetAllowsScreenSaver(true);
		
#if PLATFORM_IOS
		ZGUninstallTouchGestures(window);
		showGameMenus(window);
#endif
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
	if (gGameState == GAME_STATE_ON || gGameState == GAME_STATE_TUTORIAL || gGameState == GAME_STATE_PAUSED)
	{
		// Render opaque objects first
		
		pushDebugGroup(renderer, "Weapons");
		
		// Weapons renders at z = -24.0f to -25.0f after a world rotation
		drawWeapon(renderer, gRedRover.weap);
		drawWeapon(renderer, gGreenTree.weap);
		drawWeapon(renderer, gPinkBubbleGum.weap);
		drawWeapon(renderer, gBlueLightning.weap);
		
		popDebugGroup(renderer);

		// Characters renders at z = -25.3 to -24.7 after a world rotation when not fallen
		// When falling, will reach to around -195
		pushDebugGroup(renderer, "Characters");
		drawCharacters(renderer, RENDERER_OPTION_NONE);
		popDebugGroup(renderer);

		// Tiles renders at z = -25.0f to -26.0f after a world rotation when not fallen
		pushDebugGroup(renderer, "Tiles");
		drawTiles(renderer);
		popDebugGroup(renderer);
		
		// Character icons at the bottom of the screen at z = -25.0f
		const mat4_t characterIconTranslations[] =
		{
			m4_translation((vec3_t){CHARACTER_ICON_OFFSET, -9.2f, -25.0f}),
			m4_translation((vec3_t){CHARACTER_ICON_OFFSET + CHARACTER_ICON_DISPLACEMENT, -9.2f, -25.0f}),
			m4_translation((vec3_t){CHARACTER_ICON_OFFSET + CHARACTER_ICON_DISPLACEMENT * 2, -9.2f, -25.0f}),
			m4_translation((vec3_t){CHARACTER_ICON_OFFSET + CHARACTER_ICON_DISPLACEMENT * 3, -9.2f, -25.0f})
		};
		pushDebugGroup(renderer, "Icons");
		drawCharacterIcons(renderer, characterIconTranslations);
		popDebugGroup(renderer);
		
		// Render transparent objects from zFar to zNear
		
		// Sky renders at z = -38.0f
		pushDebugGroup(renderer, "Sky");
		drawSky(renderer, RENDERER_OPTION_BLENDING_ALPHA);
		popDebugGroup(renderer);
		
		// Characters renders at z = -25.3 to -24.7 after a world rotation when not fallen
		// When falling, will reach to around -195
		pushDebugGroup(renderer, "Characters");
		drawCharacters(renderer, RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA);
		popDebugGroup(renderer);
		
		// Character lives at z = -25.0f
		pushDebugGroup(renderer, "Character Info");
		bool displayControllerNames = (!gGameHasStarted || gGameState == GAME_STATE_PAUSED) && gNetworkConnection == NULL;
		drawAllCharacterInfo(renderer, characterIconTranslations, displayControllerNames);
		popDebugGroup(renderer);
		
		// Render touch input visuals at z = -25.0f
#if PLATFORM_IOS
		if (gGameState == GAME_STATE_TUTORIAL && gTutorialStage >= 1 && gTutorialStage < 5)
		{
			pushDebugGroup(renderer, "Touch Input");
			
			const ZGFloat arrowOffset = 1.0f;
			const ZGFloat arrowScale = 0.004f;
			
			color4_t defaultArrowColor = (color4_t){0.0f, 0.0f, 1.0f, 1.0f};
			color4_t activeArrowColor = (color4_t){gPinkBubbleGum.red, gPinkBubbleGum.green, gPinkBubbleGum.blue, 1.0f};
			
			Character *humanCharacter = NULL;
			if (gPinkBubbleGum.state == CHARACTER_HUMAN_STATE)
			{
				humanCharacter = &gPinkBubbleGum;
			}
			else if (gRedRover.state == CHARACTER_HUMAN_STATE)
			{
				humanCharacter = &gPinkBubbleGum;
			}
			else if (gGreenTree.state == CHARACTER_HUMAN_STATE)
			{
				humanCharacter = &gGreenTree;
			}
			else
			{
				humanCharacter = &gBlueLightning;
			}
			
			//-10.792 to 10.792 for z=-25.0f without aspect ratio taken in account in x direction
			const float scaleX = 10.792f * computeProjectionAspectRatio(renderer);
			ZGFloat touchInputX = (ZGFloat)(scaleX * 2 * 0.1f + -scaleX);
			ZGFloat touchInputY = -1.0f;
			
			mat4_t upwardMatrix = m4_translation((vec3_t){touchInputX, touchInputY + arrowOffset, -25.0f});
			drawStringScaled(renderer, upwardMatrix, (humanCharacter->direction == UP ? activeArrowColor : defaultArrowColor), arrowScale, "↑");
			
			mat4_t downwardMatrix = m4_translation((vec3_t){touchInputX, touchInputY - arrowOffset, -25.0f});
			drawStringScaled(renderer, downwardMatrix, (humanCharacter->direction == DOWN ? activeArrowColor : defaultArrowColor), arrowScale, "↓");
			
			mat4_t rightwardMatrix = m4_translation((vec3_t){touchInputX + arrowOffset, touchInputY, -25.0f});
			drawStringScaled(renderer, rightwardMatrix, (humanCharacter->direction == RIGHT ? activeArrowColor : defaultArrowColor), arrowScale, "→");
			
			mat4_t leftwardMatrix = m4_translation((vec3_t){touchInputX - arrowOffset, touchInputY, -25.0f});
			drawStringScaled(renderer, leftwardMatrix, (humanCharacter->direction == LEFT ? activeArrowColor : defaultArrowColor), arrowScale, "←");

			popDebugGroup(renderer);
		}
		
		if (gGameState == GAME_STATE_TUTORIAL && gTutorialStage >= 3 && gTutorialStage < 5)
		{
			//-10.792 to 10.792 for z=-25.0f without aspect ratio taken in account in x direction
			const float scaleX = 10.792f * computeProjectionAspectRatio(renderer);
			ZGFloat tapInputX = (ZGFloat)(scaleX * 2 * 0.9f + -scaleX);
			ZGFloat tapInputY = -1.0f;
			
			Character *humanCharacter = NULL;
			if (gPinkBubbleGum.state == CHARACTER_HUMAN_STATE)
			{
				humanCharacter = &gPinkBubbleGum;
			}
			else if (gRedRover.state == CHARACTER_HUMAN_STATE)
			{
				humanCharacter = &gPinkBubbleGum;
			}
			else if (gGreenTree.state == CHARACTER_HUMAN_STATE)
			{
				humanCharacter = &gGreenTree;
			}
			else
			{
				humanCharacter = &gBlueLightning;
			}
			
			mat4_t tapMatrix = m4_translation((vec3_t){tapInputX, tapInputY, -25.0f});
			drawStringScaled(renderer, tapMatrix, (color4_t){0.0f, 0.0f, 1.0f, humanCharacter->weap->animationState ? 0.5f : 1.0f}, 0.006f, "🔘");
		}
#endif
		
		// Render game instruction at -25.0f
		pushDebugGroup(renderer, "Instructional Text");
		{
			mat4_t modelViewMatrix = m4_translation((vec3_t){-1.0f / 11.2f, 80.0f / 11.2f, -280.0f / 11.2f});
			
			if (gGameState == GAME_STATE_TUTORIAL)
			{
#if PLATFORM_IOS
				ZGFloat scale = 0.0028f;
#else
				ZGFloat scale = 0.004f;
#endif
				color4_t textColor = (color4_t){gPinkBubbleGum.red, gPinkBubbleGum.green, gPinkBubbleGum.blue, 1.0f};
				
				mat4_t tutorialModelViewMatrix = m4_mul(m4_translation((vec3_t){0.0f, 0.0f, 0.0f}), modelViewMatrix);
				
				if (gTutorialStage == 0)
				{
#if PLATFORM_IOS
					ZGFloat welcomeScale = scale * 1.5f;
#else
					ZGFloat welcomeScale = scale;
#endif
					drawStringScaled(renderer, tutorialModelViewMatrix, textColor, welcomeScale, "Welcome to the Tutorial!");
				}
				else if (gTutorialStage == 1)
				{
#if PLATFORM_IOS
					const char *text = "Touch outside the board.";
					ZGFloat moveScale = scale * 1.2f;
#else
					const char *text = "Move around ⬆️➡️⬇️⬅️.";
					ZGFloat moveScale = scale;
#endif
					drawStringScaled(renderer, tutorialModelViewMatrix, textColor, moveScale, text);
					
#if PLATFORM_IOS
					const char *subtext = "Move ↑→↓←. Keep Finger held down.";
					mat4_t tutorialSubtextModelViewMatrix = m4_mul(m4_translation((vec3_t){0.0f, -1.3f, 0.0f}), tutorialModelViewMatrix);
					drawStringScaled(renderer, tutorialSubtextModelViewMatrix, textColor, scale, subtext);
#endif
				}
				else if (gTutorialStage == 2)
				{
#if PLATFORM_IOS
					ZGFloat moveScale = scale * 1.4f;
#else
					ZGFloat moveScale = scale;
#endif
					const char *text = "Move and Turn without stopping.";
					drawStringScaled(renderer, tutorialModelViewMatrix, textColor, moveScale, text);

#if PLATFORM_IOS
					const char *subtext = "Minimize Touch movement";
					mat4_t tutorialSubtextModelViewMatrix = m4_mul(m4_translation((vec3_t){0.0f, -1.3f, 0.0f}), tutorialModelViewMatrix);
					drawStringScaled(renderer, tutorialSubtextModelViewMatrix, textColor, moveScale, subtext);
#endif
				}
				if (gTutorialStage == 3)
				{
#if PLATFORM_IOS
					ZGFloat fireScale = scale * 1.4f;
					const char *text = "Tap with another Finger to Fire.";
#else
					ZGFloat fireScale = scale;
					const char *text = "Fire with spacebar.";
#endif
					drawStringScaled(renderer, tutorialModelViewMatrix, textColor, fireScale, text);
				}
				else if (gTutorialStage == 4)
				{
#if PLATFORM_IOS
					ZGFloat knockOffScale = scale * 1.5f;
#else
					ZGFloat knockOffScale = scale;
#endif
					drawStringScaled(renderer, tutorialModelViewMatrix, textColor, knockOffScale, "Knock everyone off!");
				}
				else if (gTutorialStage == 5)
				{
#if PLATFORM_IOS
					const char *text = "You're a pro! No more visuals.";
					ZGFloat endTextScale = scale * 1.5f;
#else
					const char *text = "You're a pro! Escape to exit.";
					ZGFloat endTextScale = scale;
#endif
					drawStringScaled(renderer, tutorialModelViewMatrix, textColor, endTextScale, text);
					
#if PLATFORM_IOS
					const char *subtext = "Pause to exit.";
					mat4_t tutorialSubtextModelViewMatrix = m4_mul(m4_translation((vec3_t){0.0f, -1.3f, 0.0f}), tutorialModelViewMatrix);
					drawStringScaled(renderer, tutorialSubtextModelViewMatrix, textColor, endTextScale, subtext);
#endif
				}
			}
			else if (!gGameHasStarted)
			{
				ZGFloat scale = 0.004f;
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
							
							mat4_t leftAlignedModelViewMatrix = m4_mul(m4_translation((vec3_t){-6.8f, 1.2f, 0.0f}), modelViewMatrix);
							
							drawStringLeftAligned(renderer, leftAlignedModelViewMatrix, textColor, scale, buffer);
						}
						else if (gNetworkConnection->numberOfPlayersToWaitFor == 0)
						{
							mat4_t leftAlignedModelViewMatrix = m4_mul(m4_translation((vec3_t){-6.8f, 1.2f, 0.0f}), modelViewMatrix);
							
							drawStringLeftAligned(renderer, leftAlignedModelViewMatrix, textColor, scale, "Waiting for players to connect...");
						}
						else
						{
							mat4_t leftAlignedModelViewMatrix = m4_mul(m4_translation((vec3_t){-6.8f, 1.2f, 0.0f}), modelViewMatrix);
							
							drawStringLeftAligned(renderer, leftAlignedModelViewMatrix, textColor, scale, "Waiting for 1 player to connect...");
						}
						
						if (gNetworkConnection->type == NETWORK_SERVER_TYPE && strlen(gNetworkConnection->ipAddress) > 0)
						{
							float yLocation = gGameState == GAME_STATE_PAUSED ? -2.0f : -0.2f;
							mat4_t scaledModelViewMatrix = m4_mul(m4_translation((vec3_t){0.0f, yLocation, 0.0f}), modelViewMatrix);
							
							char hostAddressDescription[256] = {0};
							snprintf(hostAddressDescription, sizeof(hostAddressDescription) - 1, "Address: %s", gNetworkConnection->ipAddress);
							drawStringScaled(renderer, scaledModelViewMatrix, (color4_t){gPinkBubbleGum.red, gPinkBubbleGum.green, gPinkBubbleGum.blue, 1.0f}, 0.003f, hostAddressDescription);
						}
					}
				}
				
				else if (gGameStartNumber > 0)
				{
					if (gGameState != GAME_STATE_PAUSED)
					{
						char buffer[256] = {0};
						snprintf(buffer, sizeof(buffer) - 1, "Game begins in %d", gGameStartNumber);
						
						mat4_t leftAlignedModelViewMatrix = m4_mul(m4_translation((vec3_t){-3.8f, 0.0f, 0.0f}), modelViewMatrix);
						drawStringLeftAligned(renderer, leftAlignedModelViewMatrix, textColor, scale, buffer);
					}
				}
				
				else if (gGameStartNumber == 0)
				{
					gGameHasStarted = true;
				}
			}
		}
		popDebugGroup(renderer);
		
#if !PLATFORM_IOS
		if (gConsoleActivated)
		{
			// Console at z = -25.0f
			pushDebugGroup(renderer, "Console");
			drawConsole(renderer);
			popDebugGroup(renderer);
		}
#endif
		
		if (gGameState == GAME_STATE_PAUSED)
		{
			pushDebugGroup(renderer, "Paused");
			
			// Renders at z = -22.0f
			drawBlackBox(renderer);
			
			// Paused title renders at -20.0f
			mat4_t gameTitleModelViewMatrix = m4_translation((vec3_t){0.0f, 5.4f, -20.0f});
			drawStringScaled(renderer, gameTitleModelViewMatrix, (color4_t){0.3f, 0.2f, 1.0f, 0.7f}, 0.00592f, "Paused");
			
#if !PLATFORM_IOS
			// Menus render at z = -20.0f
			pushDebugGroup(renderer, "Menus");
			drawMenus(renderer);
			popDebugGroup(renderer);
#endif
			popDebugGroup(renderer);
		}
		// Winning/Losing text at z = -25.0f
		else if (gGameWinner != NO_CHARACTER)
		{
#if !PLATFORM_IOS
			if (gConsoleActivated)
			{
				// Console text at z = -23.0f
				pushDebugGroup(renderer, "Console Text");
				drawConsoleText(renderer);
				popDebugGroup(renderer);
			}
#endif
			
			// Renders at z = -22.0f
			pushDebugGroup(renderer, "Black Box");
			drawBlackBox(renderer);
			popDebugGroup(renderer);
			
			if (gGameWinner != NO_CHARACTER)
			{
				// Character icons on scoreboard at z = -20.0f
				// This is supposed to actually be opaque, but it doesn't render properly in the GL renderer
				// if this is rendered beforehand, don't know why.
				pushDebugGroup(renderer, "Scoreboard");
				drawScoreboardForCharacters(renderer, SCOREBOARD_RENDER_ICONS);
				popDebugGroup(renderer);
			}
			
			// Renders winning/losing text at z = -20.0f
			mat4_t winLoseModelViewMatrix = m4_translation((vec3_t){0.0f / 1.25f, 100.0f / 14.0f, -25.0f / 1.25f});
			
			pushDebugGroup(renderer, "Winner Text");
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
			popDebugGroup(renderer);
			
			// Character scores on scoreboard at z = -20.0f
			pushDebugGroup(renderer, "Scoreboard");
			drawScoreboardForCharacters(renderer, SCOREBOARD_RENDER_SCORES);
			popDebugGroup(renderer);
			
			// Play again or exit text at z = -20.0f
			{
				mat4_t modelViewMatrix = m4_translation((vec3_t){0.0f / 1.25f, -7.0f / 1.25f, -25.0f / 1.25f});
				
				if (gNetworkConnection == NULL || gNetworkConnection->type == NETWORK_SERVER_TYPE)
				{
					// Draw a "Press ENTER to play again" notice
					pushDebugGroup(renderer, "Play Again Text");
					drawStringScaled(renderer, modelViewMatrix, (color4_t){0.2f, 0.2f, 0.6f, 1.0f}, 0.004f, "Fire to play again");
					popDebugGroup(renderer);
				}
			}
		}
		else
		{
#if !PLATFORM_IOS
			if (gConsoleActivated)
			{
				// Console text at z =  -24.0f
				pushDebugGroup(renderer, "Console Text");
				drawConsoleText(renderer);
				popDebugGroup(renderer);
			}
#endif
		}
		
#if PLATFORM_IOS
		// Pause button renders at z = -20.0f
		if (gGameState == GAME_STATE_ON || gGameState == GAME_STATE_TUTORIAL)
		{
			mat4_t gameTitleModelViewMatrix = m4_translation((vec3_t){7.5f * computeProjectionAspectRatio(renderer), 7.5f, -20.0f});
			
			pushDebugGroup(renderer, "Pause Button");
			drawStringScaled(renderer, gameTitleModelViewMatrix, (color4_t){1.0f, 1.0f, 1.0f, 0.4f}, 0.003f, "⏸️");
			popDebugGroup(renderer);
		}
#endif
		
		if (gDrawFPS)
		{
			// FPS renders at z = -18.0f
			pushDebugGroup(renderer, "FPS Text");
			drawFramesPerSecond(renderer);
			popDebugGroup(renderer);
		}
		
		if (gDrawPings)
		{
			// Pings render at z = -18.0f
			pushDebugGroup(renderer, "Pings");
			drawPings(renderer);
			popDebugGroup(renderer);
		}
	}
	else /* if (gGameState != GAME_STATE_ON && gGameState != GAME_STATE_TUTORIAL && gGameState != GAME_STATE_PAUSED) */
	{
		// The game title and menu's should be up front the most
		// The black box should be behind the title and menu's
		// The sky should be behind the black box
		
		// Sky renders at -38.0f
		pushDebugGroup(renderer, "Sky");
		drawSky(renderer, RENDERER_OPTION_DISABLE_DEPTH_TEST);
		popDebugGroup(renderer);
		
		// Black box renders at -22.0f
		pushDebugGroup(renderer, "Black Box");
		drawBlackBox(renderer);
		popDebugGroup(renderer);
		
		// Title renders at -20.0f
		pushDebugGroup(renderer, "Game Title");
		mat4_t gameTitleModelViewMatrix = m4_translation((vec3_t){0.0f, 5.4f, -20.0f});
		drawStringScaled(renderer, gameTitleModelViewMatrix, (color4_t){0.3f, 0.2f, 1.0f, 0.7f}, 0.00592f, "Sky Checkers");
		popDebugGroup(renderer);
		
		if (gGameState == GAME_STATE_CONNECTING)
		{
			// Rendering connecting to server at z = -20.0f
			mat4_t translationMatrix = m4_translation((vec3_t){-1.0f / 14.0f, 15.0f / 14.0f, -280.0f / 14.0f});
			color4_t textColor = (color4_t){0.3f, 0.2f, 1.0f, 1.0f};
			
			pushDebugGroup(renderer, "Connecting Text");
			drawString(renderer, translationMatrix, textColor, 50.0f / 14.0f, 5.0f / 14.0f, "Connecting to server...");
			popDebugGroup(renderer);
		}
		else /* if (gGameState == GAME_STATE_OFF) */
		{
#if !PLATFORM_IOS
			// Menus render at z = -20.0f
			pushDebugGroup(renderer, "Menus");
			drawMenus(renderer);
			popDebugGroup(renderer);
#endif
		}
		
		if (gDrawFPS)
		{
			// FPS renders at z = -18.0f
			pushDebugGroup(renderer, "FPS Text");
			drawFramesPerSecond(renderer);
			popDebugGroup(renderer);
		}
	}
}

#if !PLATFORM_IOS
static void handleKeyDownEvent(ZGKeyboardEvent *event, Renderer *renderer)
{
	ZGWindow *window = renderer->window;
	
	uint16_t keyCode = event->keyCode;
	uint64_t keyModifier = event->keyModifier;
	
	if (gGameState == GAME_STATE_OFF || gGameState == GAME_STATE_PAUSED)
	{
		performKeyboardMenuAction(event, &gGameState, renderer->window);
	}
	else if (!gConsoleActivated && gGameState == GAME_STATE_ON && gGameWinner != NO_CHARACTER &&
		(keyCode == gPinkBubbleGumInput.weap_id || keyCode == gRedRoverInput.weap_id ||
		 keyCode == gBlueLightningInput.weap_id || keyCode == gGreenTreeInput.weap_id || ZGTestReturnKeyCode(keyCode)))
	{
		// new game
		if (!gNetworkConnection || gNetworkConnection->type == NETWORK_SERVER_TYPE)
		{
			resetGame();
		}
	}
	else if (ZGTestReturnKeyCode(keyCode) && gConsoleActivated)
	{
		executeConsoleCommand();
	}
	else if (keyCode == ZG_KEYCODE_ESCAPE)
	{
		if (gGameState == GAME_STATE_ON || gGameState == GAME_STATE_TUTORIAL)
		{
			if (gConsoleActivated)
			{
				clearConsole();
			}
			else
			{
				showPauseMenu(window, &gGameState);
			}
		}
	}
	else if (keyCode == ZG_KEYCODE_GRAVE && gGameState == GAME_STATE_ON)
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
	else if (keyCode == ZG_KEYCODE_BACKSPACE && gGameState == GAME_STATE_ON)
	{
		if (gConsoleActivated)
		{
			performConsoleBackspace();
		}
	}
	
	if (!ZGTestMetaModifier(keyModifier) && (gGameState == GAME_STATE_ON || gGameState == GAME_STATE_TUTORIAL || gGameState == GAME_STATE_PAUSED))
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

static void handleKeyUpEvent(ZGKeyboardEvent *event)
{
	uint16_t keyCode = event->keyCode;
	uint64_t keyModifier = event->keyModifier;
	
	if ((keyCode != ZG_KEYCODE_ESCAPE || !ZGTestMetaModifier(keyModifier)) && (gGameState == GAME_STATE_ON || gGameState == GAME_STATE_TUTORIAL || gGameState == GAME_STATE_PAUSED))
	{
		performUpKeyAction(&gRedRoverInput, event, gGameState);
		performUpKeyAction(&gGreenTreeInput, event, gGameState);
		performUpKeyAction(&gPinkBubbleGumInput, event, gGameState);
		performUpKeyAction(&gBlueLightningInput, event, gGameState);
	}
}

static void handleTextInputEvent(ZGKeyboardEvent *event)
{
	if (gGameState == GAME_STATE_OFF)
	{
		performKeyboardMenuTextInputAction(event);
	}
	else if (gGameState == GAME_STATE_ON && gConsoleActivated)
	{
		char *text = event->text;
		for (uint8_t textIndex = 0; textIndex < sizeof(event->text); textIndex++)
		{
			if (text[textIndex] == 0x0 || text[textIndex] == 0x1 || text[textIndex] == '`' || text[textIndex] == '~')
			{
				break;
			}
			
			writeConsoleText((uint8_t)text[textIndex]);
		}
	}
}
#endif

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

static void pollGamepads(GamepadManager *gamepadManager, ZGWindow *window, const void *systemEvent)
{
	uint16_t gamepadEventsCount = 0;
	GamepadEvent *gamepadEvents = pollGamepadEvents(gamepadManager, systemEvent, &gamepadEventsCount);
	for (uint16_t gamepadEventIndex = 0; gamepadEventIndex < gamepadEventsCount; gamepadEventIndex++)
	{
		GamepadEvent *gamepadEvent = &gamepadEvents[gamepadEventIndex];
		
		if (gGameState == GAME_STATE_OFF || gGameState == GAME_STATE_PAUSED)
		{
			performGamepadMenuAction(gamepadEvent, &gGameState, window);
		}
		else if ((gGameState == GAME_STATE_ON || gGameState == GAME_STATE_TUTORIAL) && gamepadEvent->state == GAMEPAD_STATE_PRESSED && (gamepadEvent->button == GAMEPAD_BUTTON_START || gamepadEvent->button == GAMEPAD_BUTTON_BACK))
		{
			showPauseMenu(window, &gGameState);
		}
		
		performGamepadAction(&gRedRoverInput, gamepadEvent, gGameState);
		performGamepadAction(&gGreenTreeInput, gamepadEvent, gGameState);
		performGamepadAction(&gPinkBubbleGumInput, gamepadEvent, gGameState);
		performGamepadAction(&gBlueLightningInput, gamepadEvent, gGameState);
	}
	
#if !PLATFORM_IOS
	bool consoleActivated = gConsoleActivated;
#else
	bool consoleActivated = false;
#endif
	if (!consoleActivated && gGameState == GAME_STATE_ON && gGameWinner != NO_CHARACTER && (gNetworkConnection == NULL || gNetworkConnection->type == NETWORK_SERVER_TYPE) && (gRedRoverInput.weap || gGreenTreeInput.weap || gPinkBubbleGumInput.weap || gBlueLightningInput.weap))
	{
		resetGame();
	}
}

static void handleWindowEvent(ZGWindowEvent event, void *context)
{
	AppContext *appContext = context;
	switch (event.type)
	{
		case ZGWindowEventTypeResize:
			updateViewport(&appContext->renderer, event.width, event.height);
			break;
		case ZGWindowEventTypeFocusGained:
			if (gGameState == GAME_STATE_OFF || gGameState == GAME_STATE_CONNECTING)
			{
				unPauseMusic();
			}
			break;
		case ZGWindowEventTypeFocusLost:
			if (gGameState == GAME_STATE_OFF || gGameState == GAME_STATE_CONNECTING)
			{
				pauseMusic();
			}
			else
			{
				showPauseMenu(appContext->renderer.window, &gGameState);
			}
			break;
		case ZGWindowEventTypeShown:
			appContext->needsToDrawScene = true;
			appContext->lastRunloopTime = 0;
			break;
		case ZGWindowEventTypeHidden:
			appContext->needsToDrawScene = false;
			appContext->lastRunloopTime = 0;
			break;
	}
}

#if PLATFORM_IOS
static void handleTouchEvent(ZGTouchEvent event, void *context)
{
	if (gGameState == GAME_STATE_ON || gGameState == GAME_STATE_TUTORIAL)
	{
		Renderer *renderer = context;
		
		if (event.type == ZGTouchEventTypeTap && event.x >= (float)renderer->windowWidth - (renderer->windowWidth * 0.11f) && event.y <= (renderer->windowHeight * 0.11f))
		{
			showPauseMenu(renderer->window, &gGameState);
		}
		else if (event.type == ZGTouchEventTypeTap && gGameWinner != NO_CHARACTER && gGameState == GAME_STATE_ON)
		{
			resetGame();
		}
		else
		{
			performTouchAction(&gPinkBubbleGumInput, &event);
		}
	}
}
#else
static void handleKeyboardEvent(ZGKeyboardEvent event, void *context)
{
	switch (event.type)
	{
		case ZGKeyboardEventTypeKeyDown:
			handleKeyDownEvent(&event, context);
			break;
		case ZGKeyboardEventTypeKeyUp:
			handleKeyUpEvent(&event);
			break;
		case ZGKeyboardEventTypeTextInput:
			handleTextInputEvent(&event);
			break;
	}
}
#endif

static void gamepadAdded(GamepadIndex gamepadIndex, void *context)
{
	for (uint16_t index = 0; index < sizeof(gGamepads) / sizeof(*gGamepads); index++)
	{
		if (gGamepads[index] == INVALID_GAMEPAD_INDEX)
		{
			gGamepads[index] = gamepadIndex;
			
			if (gGameState != GAME_STATE_OFF)
			{
				Input *input = NULL;
				int64_t playerIndex = 0;
				if (gBlueLightningInput.gamepadIndex == INVALID_GAMEPAD_INDEX)
				{
					input = &gBlueLightningInput;
					playerIndex = 3;
				}
				
				if (gGreenTreeInput.gamepadIndex == INVALID_GAMEPAD_INDEX)
				{
					input = &gGreenTreeInput;
					playerIndex = 2;
				}
				
				if (gRedRoverInput.gamepadIndex == INVALID_GAMEPAD_INDEX)
				{
					input = &gRedRoverInput;
					playerIndex = 1;
				}
				
				if (gPinkBubbleGumInput.gamepadIndex == INVALID_GAMEPAD_INDEX)
				{
					input = &gPinkBubbleGumInput;
					playerIndex = 0;
				}
				
				if (input != NULL)
				{
					input->gamepadIndex = gamepadIndex;
					setPlayerIndex(gGamepadManager, gamepadIndex, playerIndex);
					
					if (input->character != NULL)
					{
						const char *controllerName = gamepadName(gGamepadManager, gamepadIndex);
						if (controllerName != NULL)
						{
							strncpy(input->character->controllerName, controllerName, MAX_CONTROLLER_NAME_SIZE - 1);
						}
					}
				}
			}
			else
			{
				setPlayerIndex(gGamepadManager, gamepadIndex, UNSET_PLAYER_INDEX);
			}
			break;
		}
	}
}

static void gamepadRemoved(GamepadIndex gamepadIndex, void *context)
{
	for (uint16_t index = 0; index < sizeof(gGamepads) / sizeof(*gGamepads); index++)
	{
		if (gGamepads[index] == gamepadIndex)
		{
			gGamepads[index] = INVALID_GAMEPAD_INDEX;
			
			Input *input = NULL;
			
			if (gPinkBubbleGumInput.gamepadIndex == gamepadIndex)
			{
				input = &gPinkBubbleGumInput;
			}
			else if (gRedRoverInput.gamepadIndex == gamepadIndex)
			{
				input = &gRedRoverInput;
			}
			else if (gGreenTreeInput.gamepadIndex == gamepadIndex)
			{
				input = &gGreenTreeInput;
			}
			else if (gBlueLightningInput.gamepadIndex == gamepadIndex)
			{
				input = &gBlueLightningInput;
			}
			
			if (input != NULL)
			{
				input->gamepadIndex = INVALID_GAMEPAD_INDEX;
				
				if (input->character != NULL)
				{
					memset(input->character->controllerName, 0, MAX_CONTROLLER_NAME_SIZE);
				}
			}
			
			break;
		}
	}
}

static void appLaunchedHandler(void *context)
{
	AppContext *appContext = context;
	
	appContext->lastRunloopTime = 0;
	appContext->lastFrameTime = 0.0;
	appContext->cyclesLeftOver = 0.0;
	appContext->needsToDrawScene = true;
	
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
	
	Renderer *renderer = &appContext->renderer;
	
	createRenderer(renderer, gWindowWidth, gWindowHeight, gFullscreenFlag, vsync, gFsaaFlag);
	
	initText(renderer);
	
	// Initialize game related things
	
#ifdef _PROFILING
	gDrawFPS = true;
#endif
	
	initScene(renderer);
	
	gGamepadManager = initGamepadManager("Data/gamecontrollerdb.txt", gamepadAdded, gamepadRemoved, NULL);
	
	// Create netcode buffers and mutex's in case we need them later
	initializeNetworkBuffers();
	
	if (gAudioMusicFlag)
	{
		bool windowFocus = ZGWindowHasFocus(renderer->window);
		playMainMenuMusic(!windowFocus);
	}
	
	ZGSetWindowEventHandler(renderer->window, appContext, handleWindowEvent);
#if PLATFORM_IOS
	ZGSetTouchEventHandler(renderer->window, renderer, handleTouchEvent);
#else
	ZGSetKeyboardEventHandler(renderer->window, renderer, handleKeyboardEvent);
#endif
}

static void appTerminatedHandler(void *context)
{
	AppContext *appContext = context;
	Renderer *renderer = &appContext->renderer;
	
	// Save user defaults
	writeDefaults(renderer);
	
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
			syncNetworkState(renderer->window, (float)ANIMATION_TIMER_INTERVAL);
			ZGDelay(10);
		}
	}
}

static void appSuspendedHandler(void *context)
{
	AppContext *appContext = context;
	Renderer *renderer = &appContext->renderer;
	
	showPauseMenu(renderer->window, &gGameState);
	
	writeDefaults(renderer);
}

static void runLoopHandler(void *context)
{
	AppContext *appContext = context;
	Renderer *renderer = &appContext->renderer;
	
	// Update game state
	// http://ludobloom.com/tutorials/timestep.html
	
	double currentTime = ZGGetTicks() / 1000.0;
	double updateIterations = ((currentTime - appContext->lastFrameTime) + appContext->cyclesLeftOver);
	
	if (updateIterations > MAX_ITERATIONS)
	{
		updateIterations = MAX_ITERATIONS;
	}
	
	while (updateIterations > ANIMATION_TIMER_INTERVAL)
	{
		updateIterations -= ANIMATION_TIMER_INTERVAL;
		
		syncNetworkState(renderer->window, (float)ANIMATION_TIMER_INTERVAL);
		
		if (gGameState == GAME_STATE_ON || gGameState == GAME_STATE_TUTORIAL || (gGameState == GAME_STATE_PAUSED && gNetworkConnection != NULL))
		{
			animate(renderer->window, ANIMATION_TIMER_INTERVAL, gGameState);
		}
		
		if (gGameShouldReset)
		{
			endGame(renderer->window, false);
			initGame(renderer->window, false, false);
		}
	}
	
	appContext->cyclesLeftOver = updateIterations;
	appContext->lastFrameTime = currentTime;
	
	if (appContext->needsToDrawScene)
	{
		renderFrame(renderer, drawScene);
	}
	
	bool shouldCapFPS = !appContext->needsToDrawScene || !renderer->vsync;
	if (shouldCapFPS)
	{
		uint32_t timeAfterRender = ZGGetTicks();
		
		if (appContext->lastRunloopTime > 0 && timeAfterRender > appContext->lastRunloopTime)
		{
			uint32_t timeElapsed = timeAfterRender - appContext->lastRunloopTime;
			uint32_t targetTime = (uint32_t)(1000.0 / MAX_FPS_RATE);
			
			if (timeElapsed < targetTime)
			{
				ZGDelay(targetTime - timeElapsed);
			}
		}
		
		appContext->lastRunloopTime = ZGGetTicks();
	}
}

static void pollEventHandler(void *context, void *systemEvent)
{
	AppContext *appContext = context;
	Renderer *renderer = &appContext->renderer;
	
	pollGamepads(gGamepadManager, renderer->window, systemEvent);
	ZGPollWindowAndInputEvents(renderer->window, systemEvent);
}

int main(int argc, char *argv[])
{
	AppContext appContext;
	ZGAppHandlers appHandlers = {.launchedHandler =  appLaunchedHandler, .terminatedHandler = appTerminatedHandler, .runLoopHandler = runLoopHandler, .pollEventHandler = pollEventHandler, .suspendedHandler = appSuspendedHandler};
	return ZGAppInit(argc, argv, &appHandlers, &appContext);
}
