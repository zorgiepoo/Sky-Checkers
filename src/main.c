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
#include "zgtime.h"
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

#define CURRENT_DEFAULT_VERSION 6

bool gGameHasStarted;
bool gGameShouldReset;
int32_t gGameStartNumber;
uint8_t gTutorialStage;
float gTutorialCoverTimer;
int gGameWinner;

GamepadManager *gGamepadManager;
static GamepadIndex gGamepads[12] = {INVALID_GAMEPAD_INDEX, INVALID_GAMEPAD_INDEX, INVALID_GAMEPAD_INDEX, INVALID_GAMEPAD_INDEX, INVALID_GAMEPAD_INDEX, INVALID_GAMEPAD_INDEX, INVALID_GAMEPAD_INDEX, INVALID_GAMEPAD_INDEX, INVALID_GAMEPAD_INDEX, INVALID_GAMEPAD_INDEX, INVALID_GAMEPAD_INDEX, INVALID_GAMEPAD_INDEX};

// Has the user played the game once?
bool gPlayedGame = false;

// Console flag indicating if we can use the console
bool gConsoleFlag = false;

// audio flags
bool gAudioEffectsFlag = true;
bool gAudioMusicFlag = true;

// video flags
static bool gFullscreenFlag;
static bool gFsaaFlag;
static int32_t gWindowWidth;
static int32_t gWindowHeight;

// online fields
#if PLATFORM_IOS
char gServerAddressString[MAX_SERVER_ADDRESS_SIZE] =  {0};
#else
char gServerAddressString[MAX_SERVER_ADDRESS_SIZE] = "localhost";
#endif
int gServerAddressStringIndex;

char gUserNameString[MAX_USER_NAME_SIZE];
int gUserNameStringIndex = 0;

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

static void drawTutorialCover(Renderer *renderer)
{
	static BufferArrayObject vertexArrayObject;
	static BufferObject indicesBufferObject;
	static bool initializedBuffers;
	
	if (!initializedBuffers)
	{
		const ZGFloat vertices[] =
		{
			-8.0f, 27.5f, 0.0f, 1.0,
			8.0f, 27.5f, 0.0f, 1.0,
			8.0f, 10.0f, 0.0f, 1.0,
			-8.0f, 10.0f, 0.0f, 1.0
		};
		
		vertexArrayObject = createVertexArrayObject(renderer, vertices, sizeof(vertices));
		indicesBufferObject = rectangleIndexBufferObject(renderer);
		
		initializedBuffers = true;
	}
	
	mat4_t worldRotationMatrix = m4_rotation_x(-40.0f * ((ZGFloat)M_PI / 180.0f));
	mat4_t worldTranslationMatrix = m4_translation((vec3_t){-7.0f, 12.5f, -25.0f});
	mat4_t worldMatrix = m4_mul(worldRotationMatrix, worldTranslationMatrix);
	
	mat4_t modelTranslationMatrix = m4_translation((vec3_t){7.0f, -14.0f, 4.0f});
	mat4_t finalMatrix = m4_mul(worldMatrix, modelTranslationMatrix);
	
	drawVerticesFromIndices(renderer, finalMatrix, RENDERER_TRIANGLE_MODE, vertexArrayObject, indicesBufferObject, 6, (color4_t){0.0f, 0.0f, 0.0f, 0.8f}, RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA);
}

static void initScene(Renderer *renderer)
{
	loadSceneryTextures(renderer);

	loadTiles();

	initCharacters();

	loadCharacterTextures(renderer);
	buildCharacterModels(renderer);

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

#if !PLATFORM_IOS
static int readDefaultKeyCode(Defaults defaults, const char *characterName, const char *keyType, ZGConstantKeyCode defaultKeyCode)
{
	char keyBuffer[512] = {0};
	snprintf(keyBuffer, sizeof(keyBuffer), "%s %s", characterName, keyType);
	
	return readDefaultIntKey(defaults, keyBuffer, defaultKeyCode);
}

static void readCharacterKeyboardDefaults(Defaults defaults, const char *characterName, Input *input, ZGConstantKeyCode rightDefault, ZGConstantKeyCode leftDefault, ZGConstantKeyCode upDefault, ZGConstantKeyCode downDefault, ZGConstantKeyCode weaponDefault)
{
	int right = readDefaultKeyCode(defaults, characterName, "key right", rightDefault);
	int left = readDefaultKeyCode(defaults, characterName, "key left", leftDefault);
	int up = readDefaultKeyCode(defaults, characterName, "key up", upDefault);
	int down = readDefaultKeyCode(defaults, characterName, "key down", downDefault);
	int weapon = readDefaultKeyCode(defaults, characterName, "key weapon", weaponDefault);
	
	initInput(input);
	setInputKeys(input, right, left, up, down, weapon);
}
#endif

static void readDefaultCharacterState(Defaults defaults, const char *defaultKey, Character *character, int defaultMode)
{
	character->state = readDefaultIntKey(defaults, defaultKey, defaultMode);
	if (character->state != CHARACTER_HUMAN_STATE && character->state != CHARACTER_AI_STATE)
	{
		character->state = defaultMode;
	}
}

static void readDefaults(void)
{
	Defaults defaults = userDefaultsForReading();

	// conole flag default
	gConsoleFlag = readDefaultBoolKey(defaults, "Console flag", false);

	// video defaults
	int windowWidth = readDefaultIntKey(defaults, "screen width", 800);
	int windowHeight = readDefaultIntKey(defaults, "screen height", 500);
	
	if (windowWidth <= 0 || windowHeight <= 0)
	{
		gWindowWidth = 800;
		gWindowHeight = 500;
	}
	else
	{
		gWindowWidth = windowWidth;
		gWindowHeight = windowHeight;
	}
 
	gFsaaFlag = readDefaultBoolKey(defaults, "FSAA flag", true);
	gFullscreenFlag = readDefaultBoolKey(defaults, "Fullscreen flag", false);

	gCharacterLives = readDefaultIntKey(defaults, "Number of lives", MAX_CHARACTER_LIVES / 2);
	if (gCharacterLives > MAX_CHARACTER_LIVES)
	{
		gCharacterLives = MAX_CHARACTER_LIVES;
	}
	else if (gCharacterLives < 0)
	{
		gCharacterLives = MAX_CHARACTER_LIVES / 2;
	}

	// character states
	readDefaultCharacterState(defaults, "Pink Bubblegum state", &gPinkBubbleGum, CHARACTER_HUMAN_STATE);
	readDefaultCharacterState(defaults, "Red Rover state", &gRedRover, CHARACTER_AI_STATE);
	readDefaultCharacterState(defaults, "Green Tree state", &gGreenTree, CHARACTER_AI_STATE);
	readDefaultCharacterState(defaults, "Blue Lightning state", &gBlueLightning, CHARACTER_AI_STATE);
	
	gAIMode = readDefaultIntKey(defaults, "AI Mode", AI_EASY_MODE);
	if (gAIMode != AI_EASY_MODE && gAIMode != AI_MEDIUM_MODE && gAIMode != AI_HARD_MODE)
	{
		gAIMode = AI_EASY_MODE;
	}
	
	gNumberOfNetHumans = readDefaultIntKey(defaults, "Number of Net Humans", 1);
	if (gNumberOfNetHumans < 0 || gNumberOfNetHumans > 3)
	{
		gNumberOfNetHumans = 1;
	}
	
#if !PLATFORM_IOS
	readCharacterKeyboardDefaults(defaults, "Pink Bubblegum", &gPinkBubbleGumInput, ZG_KEYCODE_RIGHT, ZG_KEYCODE_LEFT, ZG_KEYCODE_UP, ZG_KEYCODE_DOWN, ZG_KEYCODE_SPACE);
	readCharacterKeyboardDefaults(defaults, "Red Rover", &gRedRoverInput, ZG_KEYCODE_B, ZG_KEYCODE_C, ZG_KEYCODE_F, ZG_KEYCODE_V, ZG_KEYCODE_G);
	readCharacterKeyboardDefaults(defaults, "Green Tree", &gGreenTreeInput, ZG_KEYCODE_L, ZG_KEYCODE_J, ZG_KEYCODE_I, ZG_KEYCODE_K, ZG_KEYCODE_M);
	readCharacterKeyboardDefaults(defaults, "Blue Lightning", &gBlueLightningInput, ZG_KEYCODE_D, ZG_KEYCODE_A, ZG_KEYCODE_W, ZG_KEYCODE_S, ZG_KEYCODE_Z);
#endif
	
	readDefaultKey(defaults, "Server IP Address", gServerAddressString, sizeof(gServerAddressString));
#if !PLATFORM_IOS
	if (strlen(gServerAddressString) == 0)
	{
		strncpy(gServerAddressString, "localhost", sizeof(gServerAddressString) - 1);
	}
#endif
	gServerAddressStringIndex = (int)strlen(gServerAddressString);
	
	readDefaultKey(defaults, "Net name", gUserNameString, sizeof(gUserNameString));
	if (strlen(gUserNameString) == 0)
	{
#if PLATFORM_OSX
		getDefaultUserName(gUserNameString, MAX_USER_NAME_SIZE - 1);
		if (strlen(gUserNameString) == 0)
#endif
		{
			char *randomNames[] = { "Tale", "Backer", "Hop", "Expel", "Rida", "Tao", "Eyez", "Phia", "Sync", "Witty", "Poet", "Roost", "Kuro", "Spot", "Carb", "Unow", "Gil", "Needle", "Oxy", "Kale" };
			
			int randomNameIndex = (int)(mt_random() % (sizeof(randomNames) / sizeof(randomNames[0])));
			char *randomName = randomNames[randomNameIndex];
			
			strncpy(gUserNameString, randomName, MAX_USER_NAME_SIZE - 1);
		}
	}
	gUserNameStringIndex = (int)strlen(gUserNameString);
	
	gAudioEffectsFlag = readDefaultBoolKey(defaults, "Audio effects", true);
	gAudioMusicFlag = readDefaultBoolKey(defaults, "Music", true);
	
	gPlayedGame = readDefaultBoolKey(defaults, "Played", false);
	
	closeDefaults(defaults);
}

#if !PLATFORM_IOS
static void writeCharacterInput(Defaults defaults, const char *characterName, Input *input)
{
	char keyBuffer[256] = {0};
	
	snprintf(keyBuffer, sizeof(keyBuffer), "%s key right", characterName);
	writeDefaultIntKey(defaults, keyBuffer, input->r_id);
	
	snprintf(keyBuffer, sizeof(keyBuffer), "%s key left", characterName);
	writeDefaultIntKey(defaults, keyBuffer, input->l_id);
	
	snprintf(keyBuffer, sizeof(keyBuffer), "%s key up", characterName);
	writeDefaultIntKey(defaults, keyBuffer, input->u_id);
	
	snprintf(keyBuffer, sizeof(keyBuffer), "%s key down", characterName);
	writeDefaultIntKey(defaults, keyBuffer, input->d_id);
	
	snprintf(keyBuffer, sizeof(keyBuffer), "%s key weapon", characterName);
	writeDefaultIntKey(defaults, keyBuffer, input->weap_id);
}
#endif

static void writeDefaults(Renderer *renderer)
{
	Defaults defaults = userDefaultsForWriting();
	
	writeDefaultIntKey(defaults, "Defaults version", CURRENT_DEFAULT_VERSION);
	
	writeDefaultIntKey(defaults, "Console flag", gConsoleFlag);

	// video defaults
	writeDefaultIntKey(defaults, "screen width", renderer->windowWidth);
	writeDefaultIntKey(defaults, "screen height", renderer->windowHeight);
	
	writeDefaultIntKey(defaults, "FSAA flag", gFsaaFlag);
	writeDefaultIntKey(defaults, "Fullscreen flag", renderer->fullscreen);
	
	writeDefaultIntKey(defaults, "Number of lives", gCharacterLives);
	
	// character states
	writeDefaultIntKey(defaults, "Pink Bubblegum state", offlineCharacterState(&gPinkBubbleGum));
	writeDefaultIntKey(defaults, "Red Rover state", offlineCharacterState(&gRedRover));
	writeDefaultIntKey(defaults, "Green Tree state", offlineCharacterState(&gGreenTree));
	writeDefaultIntKey(defaults, "Blue Lightning state", offlineCharacterState(&gBlueLightning));
	
	
	writeDefaultIntKey(defaults, "AI Mode", gAIMode);
	writeDefaultIntKey(defaults, "Number of Net Humans", gNumberOfNetHumans);

	// Character defaults
#if !PLATFORM_IOS
	writeCharacterInput(defaults, "Pink Bubblegum", &gPinkBubbleGumInput);
	writeCharacterInput(defaults, "Red Rover", &gRedRoverInput);
	writeCharacterInput(defaults, "Green Tree", &gGreenTreeInput);
	writeCharacterInput(defaults, "Blue Lightning", &gBlueLightningInput);
#endif
	
	writeDefaultStringKey(defaults, "Server IP Address", gServerAddressString);
	writeDefaultStringKey(defaults, "Net name", gUserNameString);
	
	writeDefaultIntKey(defaults, "Audio effects", gAudioEffectsFlag);
	writeDefaultIntKey(defaults, "Music", gAudioMusicFlag);
	
	writeDefaultIntKey(defaults, "Played", gPlayedGame);

	closeDefaults(defaults);
}

void initGame(ZGWindow *window, bool firstGame, bool tutorial)
{
	gPlayedGame = true;
	
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
		
		firstHumanCharacter->x = 2.0f * 3;
		firstHumanCharacter->y = 2.0f * 2;
		turnCharacter(firstHumanCharacter, UP);
		
		// We need to spawn the other characters so they fill up available spaces
		Character *characterB = NULL;
		Character *characterC = NULL;
		Character *characterD = NULL;
		getOtherCharacters(firstHumanCharacter, &characterB, &characterC, &characterD);
		
		spawnCharacter(characterB);
		spawnCharacter(characterC);
		spawnCharacter(characterD);
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
			
			// We only care about the first 4 gamepads
			uint16_t maxGamepads = 4;
			uint16_t gamepadCount = 0;
			for (uint16_t gamepadIndex = 0; gamepadIndex < maxGamepads; gamepadIndex++)
			{
				if (gGamepads[gamepadIndex] != INVALID_GAMEPAD_INDEX)
				{
					gamepadCount++;
				}
				else
				{
					break;
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

	if (tutorial)
	{
		gGameState = GAME_STATE_TUTORIAL;
	}
	else if (gGameState != GAME_STATE_PAUSED)
	{
		gGameState = GAME_STATE_ON;
	}

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
	
	if (firstGame && gAudioMusicFlag && gGameState != GAME_STATE_PAUSED)
	{
		bool windowFocus = ZGWindowHasFocus(window);
		playGameMusic(!windowFocus);
	}
	
	if (firstGame && gGameState != GAME_STATE_PAUSED)
	{
		ZGAppSetAllowsScreenIdling(false);
#if PLATFORM_TVOS
		ZGInstallMenuGesture(window);
#elif PLATFORM_IOS
		ZGInstallTouchGestures(window);
#endif
	}
}

void endGame(ZGWindow *window, bool lastGame)
{
	gGameHasStarted = false;

	endAnimation();

	if (gGameState != GAME_STATE_PAUSED)
	{
		gGameState = GAME_STATE_OFF;
	}

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
		
		restoreAllBackupStates();
		
		if (gAudioMusicFlag)
		{
			bool windowFocus = ZGWindowHasFocus(window);
			playMainMenuMusic(!windowFocus);
		}
		
		ZGAppSetAllowsScreenIdling(true);
		
		if (gGameState != GAME_STATE_PAUSED)
		{
	#if PLATFORM_TVOS
			ZGUninstallMenuGesture(window);
	#elif PLATFORM_IOS
			ZGUninstallTouchGestures(window);
			showGameMenus(window);
	#endif
		}
	}
}

void endNetworkGame(ZGWindow *window)
{
	if (gGameState == GAME_STATE_PAUSED)
	{
		hidePauseMenu(window, &gGameState);
	}
	
	endGame(window, true);
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

static void drawScoreboardForCharacters(Renderer *renderer)
{
	mat4_t iconModelViewMatrices[] =
	{
		m4_translation((vec3_t){-6.0f / 1.25f, 7.0f / 1.25f, -25.0f / 1.25f}),
		m4_translation((vec3_t){-2.0f / 1.25f, 7.0f / 1.25f, -25.0f / 1.25f}),
		m4_translation((vec3_t){2.0f / 1.25f, 7.0f / 1.25f, -25.0f / 1.25f}),
		m4_translation((vec3_t){6.0f / 1.25f, 7.0f / 1.25f, -25.0f / 1.25f})
	};
	
	drawCharacterIcons(renderer, iconModelViewMatrices);
	
	drawScoreboardTextForCharacter(renderer, &gPinkBubbleGum, iconModelViewMatrices[0]);
	drawScoreboardTextForCharacter(renderer, &gRedRover, iconModelViewMatrices[1]);
	drawScoreboardTextForCharacter(renderer, &gGreenTree, iconModelViewMatrices[2]);
	drawScoreboardTextForCharacter(renderer, &gBlueLightning, iconModelViewMatrices[3]);
}

static void drawScene(Renderer *renderer)
{
	if (gGameState == GAME_STATE_ON || gGameState == GAME_STATE_TUTORIAL || gGameState == GAME_STATE_PAUSED)
	{
		// Render opaque objects first
		
		// Characters renders at z = -25.3 to -24.7 after a world rotation when not fallen
		// When falling, will reach to around -195
		pushDebugGroup(renderer, "Characters");
		drawCharacters(renderer, RENDERER_OPTION_NONE);
		popDebugGroup(renderer);

		// Tiles renders at z = -25.0f to -26.0f after a world rotation when not fallen
		pushDebugGroup(renderer, "Tiles");
		drawTiles(renderer);
		popDebugGroup(renderer);
		
		// Render transparent objects from zFar to zNear
		
		// Sky renders at z = -38.0f
		pushDebugGroup(renderer, "Sky");
		drawSky(renderer, RENDERER_OPTION_BLENDING_ALPHA);
		popDebugGroup(renderer);
		
		// Weapons renders at z = -24.0f to -25.0f after a world rotation
		pushDebugGroup(renderer, "Weapons");
		
		drawWeapon(renderer, gRedRover.weap);
		drawWeapon(renderer, gGreenTree.weap);
		drawWeapon(renderer, gPinkBubbleGum.weap);
		drawWeapon(renderer, gBlueLightning.weap);
		
		popDebugGroup(renderer);
		
		// Characters renders at z = -25.3 to -24.7 after a world rotation when not fallen
		// When falling, will reach to around -195
		pushDebugGroup(renderer, "Characters");
		drawCharacters(renderer, RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA);
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
		
		// Character lives at z = -25.0f
		pushDebugGroup(renderer, "Character Info");
		bool displayControllerNames = (!gGameHasStarted || gGameState == GAME_STATE_PAUSED) && gNetworkConnection == NULL;
		drawAllCharacterInfo(renderer, characterIconTranslations, displayControllerNames);
		popDebugGroup(renderer);
		
		// Render touch input visuals at z = -25.0f
#if PLATFORM_IOS
		if (gGameState == GAME_STATE_TUTORIAL && gTutorialStage >= 1 && gTutorialStage < 5 && gTutorialCoverTimer <= 0.0f)
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
			ZGFloat touchInputX = (ZGFloat)(scaleX * 2 * 0.11f + -scaleX);
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
		
#if !PLATFORM_TVOS
		if (gGameState == GAME_STATE_TUTORIAL && gTutorialStage >= 3 && gTutorialStage < 5 && gTutorialCoverTimer <= 0.0f)
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
#if PLATFORM_TVOS
					const char *text = "Swipe ↑→↓← to move.";
					ZGFloat moveScale = scale;
#elif PLATFORM_IOS
					const char *text = "Touch outside the board. Swipe ↑→↓←.";
					ZGFloat moveScale = scale;
#else
					const char *text = "Move with Arrow Keys.";
					ZGFloat moveScale = scale;
#endif
					drawStringScaled(renderer, tutorialModelViewMatrix, textColor, moveScale, text);
					
#if PLATFORM_IOS
					const char *subtext = "Hold swipe to move. Let go to stop.";
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
					const char *subtext = "Minimize Swipe movement.";
					mat4_t tutorialSubtextModelViewMatrix = m4_mul(m4_translation((vec3_t){0.0f, -1.3f, 0.0f}), tutorialModelViewMatrix);
					drawStringScaled(renderer, tutorialSubtextModelViewMatrix, textColor, scale, subtext);
#endif
				}
				if (gTutorialStage == 3)
				{
#if PLATFORM_TVOS
					ZGFloat fireScale = scale;
					const char *text = "Click to Fire.";
#elif PLATFORM_IOS
					ZGFloat fireScale = scale * 1.4f;
					const char *text = "Tap with secondary Finger to Fire.";
#else
					ZGFloat fireScale = scale;
					const char *text = "Fire with spacebar.";
#endif
					drawStringScaled(renderer, tutorialModelViewMatrix, textColor, fireScale, text);
				}
				else if (gTutorialStage == 4 || gTutorialStage == 5)
				{
#if PLATFORM_IOS
					ZGFloat knockOffScale = scale * 1.5f;
#else
					ZGFloat knockOffScale = scale;
#endif
					drawStringScaled(renderer, tutorialModelViewMatrix, textColor, knockOffScale, "Knock everyone off!");
					
#if PLATFORM_IOS
					if (gTutorialStage == 5)
					{
						const char *subtext = "No more visuals.";
						
						mat4_t tutorialSubtextModelViewMatrix = m4_mul(m4_translation((vec3_t){0.0f, -1.3f, 0.0f}), tutorialModelViewMatrix);
						drawStringScaled(renderer, tutorialSubtextModelViewMatrix, textColor, scale, subtext);
					}
#endif
				}
				else if (gTutorialStage == 6)
				{
					const char *text = "You're a pro!";
					ZGFloat endTextScale = scale * 1.5f;
					
					drawStringScaled(renderer, tutorialModelViewMatrix, textColor, endTextScale, text);
					
#if PLATFORM_IOS
					const char *subtext = "Pause and Exit to end the Tutorial.";
#else
					const char *subtext = "Escape to Exit the Tutorial.";
#endif
					ZGFloat subtextScale = scale;
					
					mat4_t tutorialSubtextModelViewMatrix = m4_mul(m4_translation((vec3_t){0.0f, -1.4f, 0.0f}), tutorialModelViewMatrix);
					drawStringScaled(renderer, tutorialSubtextModelViewMatrix, textColor, subtextScale, subtext);
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
							snprintf(buffer, sizeof(buffer) - 1, "Waiting for %d players to connect…", gNetworkConnection->numberOfPlayersToWaitFor);
							
							mat4_t translatedModelViewMatrix = m4_mul(m4_translation((vec3_t){0.0f, 1.2f, 0.0f}), modelViewMatrix);
							drawStringScaled(renderer, translatedModelViewMatrix, textColor, scale, buffer);
						}
						else if (gNetworkConnection->numberOfPlayersToWaitFor == 0)
						{
							mat4_t translatedModelViewMatrix = m4_mul(m4_translation((vec3_t){0.0f, 1.2f, 0.0f}), modelViewMatrix);
							drawStringScaled(renderer, translatedModelViewMatrix, textColor, scale, "Waiting for players to connect…");
						}
						else
						{
							mat4_t translatedModelViewMatrix = m4_mul(m4_translation((vec3_t){0.0f, 1.2f, 0.0f}), modelViewMatrix);
							drawStringScaled(renderer, translatedModelViewMatrix, textColor, scale, "Waiting for 1 player to connect…");
						}
						
						if (gNetworkConnection->type == NETWORK_SERVER_TYPE && strlen(gNetworkConnection->ipAddress) > 0)
						{
							float yLocation = gGameState == GAME_STATE_PAUSED ? -2.0f : -0.2f;
							mat4_t translatedModelViewMatrix = m4_mul(m4_translation((vec3_t){0.0f, yLocation, 0.0f}), modelViewMatrix);
							
							char hostAddressDescription[256] = {0};
							snprintf(hostAddressDescription, sizeof(hostAddressDescription) - 1, "Address: %s", gNetworkConnection->ipAddress);
							drawStringScaled(renderer, translatedModelViewMatrix, (color4_t){gPinkBubbleGum.red, gPinkBubbleGum.green, gPinkBubbleGum.blue, 1.0f}, 0.003f, hostAddressDescription);
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
			
			// Character scores and icons on scoreboard at z = -20.0f
			pushDebugGroup(renderer, "Scoreboard");
			drawScoreboardForCharacters(renderer);
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
		
		if (gGameState == GAME_STATE_TUTORIAL && gTutorialCoverTimer > 0.0f)
		{
			// Tutorial cover renders around z = -21.0 after a world rotation
			pushDebugGroup(renderer, "Tutorial Cover");
			drawTutorialCover(renderer);
			popDebugGroup(renderer);
		}
		
#if PLATFORM_IOS && !PLATFORM_TVOS
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
			drawString(renderer, translationMatrix, textColor, 50.0f / 14.0f, 5.0f / 14.0f, "Connecting to server…");
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
			uint8_t character = text[textIndex];
			
			if (ALLOWED_BASIC_TEXT_INPUT(character) || character == ' ' || character == '(' || character == ')')
			{
				writeConsoleText(character);
			}
			else
			{
				break;
			}
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
#if !PLATFORM_TVOS
			performGamepadMenuAction(gamepadEvent, &gGameState, window);
#endif
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

#if !PLATFORM_IOS
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
			else if (gGameState == GAME_STATE_ON || gGameState == GAME_STATE_TUTORIAL)
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
#endif

#if PLATFORM_IOS
#define TUTORIAL_BOUNDARY_PERCENT_X 0.3f
#define TUTORIAL_BOUNDARY_PERCENT_Y 0.2f
static void handleTouchEvent(ZGTouchEvent event, void *context)
{
#if PLATFORM_TVOS
	if (event.type == ZGTouchEventTypeMenuTap)
	{
		Renderer *renderer = context;
		performMenuTapAction(renderer->window, &gGameState);
	}
	else
#endif
	if (gGameState == GAME_STATE_ON || gGameState == GAME_STATE_TUTORIAL)
	{
		Renderer *renderer = context;
		
		if (event.type == ZGTouchEventTypeTap && event.x >= (float)renderer->windowWidth - (renderer->windowWidth * 0.11f) && event.y <= (renderer->windowHeight * 0.11f))
		{
			showPauseMenu(renderer->window, &gGameState);
		}
		else if (event.type == ZGTouchEventTypeTap && gGameWinner != NO_CHARACTER && gGameState == GAME_STATE_ON && (gNetworkConnection == NULL || gNetworkConnection->type == NETWORK_SERVER_TYPE))
		{
			resetGame();
		}
		else
		{
			if (gGameState == GAME_STATE_TUTORIAL && gTutorialStage < 5 && event.type != ZGTouchEventTypeTap)
			{
				// Don't allow player to use the non-recommended boundary in tutorial mode
				if ((event.x <= (float)renderer->windowWidth * TUTORIAL_BOUNDARY_PERCENT_X || event.x >= (float)renderer->windowWidth - (float)renderer->windowWidth * TUTORIAL_BOUNDARY_PERCENT_X) && (event.y <= (float)renderer->windowHeight - (float)renderer->windowHeight * TUTORIAL_BOUNDARY_PERCENT_Y && event.y >= (float)renderer->windowHeight * TUTORIAL_BOUNDARY_PERCENT_Y))
				{
					performTouchAction(&gPinkBubbleGumInput, &event);
				}
				else
				{
					// Stop touch input if we hit a bad boundary
					event.type = ZGTouchEventTypePanEnded;
					event.deltaX = 0.0f;
					event.deltaY = 0.0f;
					
					performTouchAction(&gPinkBubbleGumInput, &event);
				}
			}
			else
			{
				performTouchAction(&gPinkBubbleGumInput, &event);
			}
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

static int compareGamepads(const void *gamepad1IndexPtr, const void *gamepad2IndexPtr)
{
	GamepadIndex gamepad1Index = *(GamepadIndex *)gamepad1IndexPtr;
	GamepadIndex gamepad2Index = *(GamepadIndex *)gamepad2IndexPtr;
	
	if (gamepad1Index == INVALID_GAMEPAD_INDEX && gamepad2Index == INVALID_GAMEPAD_INDEX)
	{
		return 0;
	}
	else if (gamepad1Index == INVALID_GAMEPAD_INDEX)
	{
		return 1;
	}
	else if (gamepad2Index == INVALID_GAMEPAD_INDEX)
	{
		return -1;
	}
	
	uint8_t gamepad1Rank = gamepadRank(gGamepadManager, gamepad1Index);
	uint8_t gamepad2Rank = gamepadRank(gGamepadManager, gamepad2Index);
	
	if (gamepad1Rank < gamepad2Rank)
	{
		return 1;
	}
	else if (gamepad1Rank > gamepad2Rank)
	{
		return -1;
	}
	
	return 0;
}

static void sortGamepads(void)
{
	qsort(gGamepads, sizeof(gGamepads) / sizeof(gGamepads[0]), sizeof(gGamepads[0]), compareGamepads);
}

static void gamepadAdded(GamepadIndex gamepadIndex, void *context)
{
	for (uint16_t index = 0; index < sizeof(gGamepads) / sizeof(*gGamepads); index++)
	{
		if (gGamepads[index] == INVALID_GAMEPAD_INDEX)
		{
			gGamepads[index] = gamepadIndex;
			sortGamepads();
			
			if (gGameState != GAME_STATE_OFF)
			{
				Input *input = NULL;
				int64_t playerIndex = 0;
				if ((gNetworkConnection != NULL || gBlueLightning.state == CHARACTER_HUMAN_STATE) && gBlueLightningInput.gamepadIndex == INVALID_GAMEPAD_INDEX)
				{
					input = &gBlueLightningInput;
					playerIndex = 3;
				}
				else if ((gNetworkConnection != NULL || gGreenTree.state == CHARACTER_HUMAN_STATE) && gGreenTreeInput.gamepadIndex == INVALID_GAMEPAD_INDEX)
				{
					input = &gGreenTreeInput;
					playerIndex = 2;
				}
				else if ((gNetworkConnection != NULL || gRedRover.state == CHARACTER_HUMAN_STATE) && gRedRoverInput.gamepadIndex == INVALID_GAMEPAD_INDEX)
				{
					input = &gRedRoverInput;
					playerIndex = 1;
				}
				else if ((gNetworkConnection != NULL || gPinkBubbleGum.state == CHARACTER_HUMAN_STATE) && gPinkBubbleGumInput.gamepadIndex == INVALID_GAMEPAD_INDEX)
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
			sortGamepads();
			
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
	bool vsync = true;
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
	
#if PLATFORM_IOS
	ZGSetTouchEventHandler(renderer->window, renderer, handleTouchEvent);
#else
	ZGSetWindowEventHandler(renderer->window, appContext, handleWindowEvent);
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
			syncNetworkState(renderer->window, (float)ANIMATION_TIMER_INTERVAL, gGameState);
			ZGDelay(10);
		}
	}
}

static void appSuspendedHandler(void *context)
{
	AppContext *appContext = context;
	Renderer *renderer = &appContext->renderer;
	
	if (gGameState == GAME_STATE_ON || gGameState == GAME_STATE_TUTORIAL)
	{
		showPauseMenu(renderer->window, &gGameState);
	}
	
	writeDefaults(renderer);
}

static void runLoopHandler(void *context)
{
	AppContext *appContext = context;
	Renderer *renderer = &appContext->renderer;
	
	// Update game state
	// http://ludobloom.com/tutorials/timestep.html
	
	double currentTime = (double)ZGGetTicks() / 1000.0;
	
	double updateIterations = ((currentTime - appContext->lastFrameTime) + appContext->cyclesLeftOver);
	
	if (updateIterations > MAX_ITERATIONS)
	{
		updateIterations = MAX_ITERATIONS;
	}
	
	while (updateIterations > ANIMATION_TIMER_INTERVAL)
	{
		updateIterations -= ANIMATION_TIMER_INTERVAL;
		
		syncNetworkState(renderer->window, (float)ANIMATION_TIMER_INTERVAL, gGameState);
		
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
#if PLATFORM_WINDOWS || PLATFORM_LINUX
	ZGPollWindowAndInputEvents(renderer->window, systemEvent);
#endif
}

int main(int argc, char *argv[])
{
	AppContext appContext;
	ZGAppHandlers appHandlers = {.launchedHandler =  appLaunchedHandler, .terminatedHandler = appTerminatedHandler, .runLoopHandler = runLoopHandler, .pollEventHandler = pollEventHandler, .suspendedHandler = appSuspendedHandler};
	return ZGAppInit(argc, argv, &appHandlers, &appContext);
}
