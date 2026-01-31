/*
 MIT License

 Copyright (c) 2019 Mayur Pawashe

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

#include <SDL3/SDL.h>

#include "gamepad.h"
#include "zgtime.h"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define INVALID_JOYSTICK_INSTANCE_ID 0

#define AXIS_THRESHOLD (Sint16)(32767 * AXIS_THRESHOLD_PERCENT)

typedef struct
{
	SDL_JoystickID joystickInstanceID;

	GamepadState lastLeftAxisState;
	GamepadState lastRightAxisState;
	GamepadState lastUpAxisState;
	GamepadState lastDownAxisState;

	bool dpadAxis;
} Gamepad;

struct _GamepadManager
{
	GamepadCallback addedCallback;
	GamepadCallback removalCallback;
	void *context;
	GamepadEvent eventsBuffer[2];
	Gamepad gamepads[MAX_GAMEPADS];
};

static void _notifyGamepadAdded(GamepadManager *gamepadManager, SDL_JoystickID joystickInstanceID)
{
	if (SDL_IsGamepad(joystickInstanceID))
	{
		// Should we close the gamepad if we fail to find an available slot?
		SDL_Gamepad *controller = SDL_OpenGamepad(joystickInstanceID);
		if (controller == NULL)
		{
			fprintf(stderr, "Failed to open gamepad %d: %s\n", joystickInstanceID, SDL_GetError());
		}
		else
		{
			bool foundExistingGamepad = false;
			for (uint32_t gamepadIndex = 0; gamepadIndex < MAX_GAMEPADS; gamepadIndex++)
			{
				if (gamepadManager->gamepads[gamepadIndex].joystickInstanceID == joystickInstanceID)
				{
					foundExistingGamepad = true;
					break;
				}
			}

			if (!foundExistingGamepad)
			{
				for (uint32_t gamepadIndex = 0; gamepadIndex < MAX_GAMEPADS; gamepadIndex++)
				{
					if (gamepadManager->gamepads[gamepadIndex].joystickInstanceID == INVALID_JOYSTICK_INSTANCE_ID)
					{
						gamepadManager->gamepads[gamepadIndex].joystickInstanceID = joystickInstanceID;
						gamepadManager->gamepads[gamepadIndex].dpadAxis = false;

						char* controllerMapping = SDL_GetGamepadMapping(controller);
						if (controllerMapping != NULL)
						{
							const char* prefixString = "dpdown:";
							char* substring = strstr(controllerMapping, prefixString);
							if (substring != NULL)
							{
								char* codeString = substring + sizeof(prefixString);
								size_t codeStringLength = strlen(codeString);
								if ((codeStringLength > 0 && codeString[0] == 'a') || (codeStringLength > 1 && codeString[0] == '+' && codeString[1] == 'a') || (codeStringLength > 1 && codeString[0] == '-' && codeString[1] == 'a'))
								{
									gamepadManager->gamepads[gamepadIndex].dpadAxis = true;
								}
							}
							SDL_free(controllerMapping);
						}

						break;
					}
				}

				if (gamepadManager->addedCallback != NULL)
				{
					gamepadManager->addedCallback((GamepadIndex)joystickInstanceID, gamepadManager->context);
				}
			}
		}
	}
}

GamepadManager *initGamepadManager(const char *databasePath, GamepadCallback addedCallback, GamepadCallback removalCallback, void *context)
{
	if (!SDL_Init(SDL_INIT_GAMEPAD))
	{
		fprintf(stderr, "Failed to initialize SDL gamepad: %s\n", SDL_GetError());
		return NULL;
	}
	
	if (SDL_AddGamepadMappingsFromFile(databasePath) == -1)
	{
		fprintf(stderr, "Failed to add SDL gamepad mappings: %s\n", SDL_GetError());
	}
	
	GamepadManager *gamepadManager = calloc(1, sizeof(*gamepadManager));
	for (uint32_t gamepadIndex = 0; gamepadIndex < MAX_GAMEPADS; gamepadIndex++)
	{
		gamepadManager->gamepads[gamepadIndex].joystickInstanceID = INVALID_JOYSTICK_INSTANCE_ID;
	}
	
	gamepadManager->addedCallback = addedCallback;
	gamepadManager->removalCallback = removalCallback;
	gamepadManager->context = context;
	
	// Scan already connected devices
	int numberOfConnectedJoysticks = 0;
	SDL_JoystickID *joystickIDs = SDL_GetJoysticks(&numberOfConnectedJoysticks);

	for (int joystickIndex = 0; joystickIndex < numberOfConnectedJoysticks; joystickIndex++)
	{
		SDL_JoystickID joystickInstanceID = joystickIDs[joystickIndex];
		_notifyGamepadAdded(gamepadManager, joystickInstanceID);
	}
	
	return gamepadManager;
}

GamepadButton _gamepadButtonForSDLButton(Uint8 sdlButton)
{
	switch (sdlButton)
	{
		case SDL_GAMEPAD_BUTTON_SOUTH:
			return GAMEPAD_BUTTON_A;
		case SDL_GAMEPAD_BUTTON_EAST:
			return GAMEPAD_BUTTON_B;
		case SDL_GAMEPAD_BUTTON_WEST:
			return GAMEPAD_BUTTON_X;
		case SDL_GAMEPAD_BUTTON_NORTH:
			return GAMEPAD_BUTTON_Y;
		case SDL_GAMEPAD_BUTTON_BACK:
			return GAMEPAD_BUTTON_BACK;
		case SDL_GAMEPAD_BUTTON_START:
			return GAMEPAD_BUTTON_START;
		case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
			return GAMEPAD_BUTTON_LEFTSHOULDER;
		case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
			return GAMEPAD_BUTTON_RIGHTSHOULDER;
		case SDL_GAMEPAD_BUTTON_DPAD_UP:
			return GAMEPAD_BUTTON_DPAD_UP;
		case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
			return GAMEPAD_BUTTON_DPAD_DOWN;
		case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
			return GAMEPAD_BUTTON_DPAD_LEFT;
		case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
			return GAMEPAD_BUTTON_DPAD_RIGHT;
		case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
		case SDL_GAMEPAD_BUTTON_LEFT_STICK:
		case SDL_GAMEPAD_BUTTON_GUIDE:
		case (Uint8)SDL_GAMEPAD_BUTTON_INVALID:
		case SDL_GAMEPAD_BUTTON_COUNT:
		default:
			return GAMEPAD_BUTTON_MAX;
	}
}

static void _addButtonEventIfNeeded(GamepadIndex gamepadIndex, GamepadButton button, GamepadElementMappingType mappingType, uint64_t timestamp, bool buttonPressed, GamepadState *lastState, GamepadEvent* eventsBuffer, uint16_t* eventIndex)
{
	GamepadState newState = (buttonPressed ? GAMEPAD_STATE_PRESSED : GAMEPAD_STATE_RELEASED);
	if (lastState == NULL || *lastState != newState)
	{
		if (lastState != NULL)
		{
			*lastState = newState;
		}

		GamepadEvent event;
		event.button = button;
		event.state = newState;
		event.mappingType = mappingType;
		event.ticks = timestamp;
		event.index = gamepadIndex;

		eventsBuffer[*eventIndex] = event;
		(*eventIndex)++;
	}
}

GamepadEvent *pollGamepadEvents(GamepadManager *gamepadManager, const void *systemEvent, uint16_t *eventCount)
{
	*eventCount = 0;
	
	if (systemEvent == NULL)
	{
		return NULL;
	}
	
	GamepadEvent *eventsBuffer = gamepadManager->eventsBuffer;
	
	const SDL_Event *sdlEvent = systemEvent;
	
	switch (sdlEvent->type)
	{
		case SDL_EVENT_GAMEPAD_ADDED:
			_notifyGamepadAdded(gamepadManager, sdlEvent->gdevice.which);
			break;
		case SDL_EVENT_GAMEPAD_REMOVED:
		{
			SDL_JoystickID joystickInstanceID = sdlEvent->gdevice.which;
			SDL_Gamepad *controller = SDL_GetGamepadFromID(joystickInstanceID);

			if (controller != NULL)
			{
				SDL_CloseGamepad(controller);
			}
			
			for (uint32_t gamepadIndex = 0; gamepadIndex < MAX_GAMEPADS; gamepadIndex++)
			{
				if (gamepadManager->gamepads[gamepadIndex].joystickInstanceID == joystickInstanceID)
				{
					gamepadManager->gamepads[gamepadIndex].joystickInstanceID = INVALID_JOYSTICK_INSTANCE_ID;
					break;
				}
			}

			if (gamepadManager->removalCallback != NULL)
			{
				gamepadManager->removalCallback((GamepadIndex)joystickInstanceID, gamepadManager->context);
			}
			break;
		}
		case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		case SDL_EVENT_GAMEPAD_BUTTON_UP:
		{
			SDL_JoystickID joystickInstanceID = sdlEvent->gbutton.which;
			GamepadButton button = _gamepadButtonForSDLButton(sdlEvent->gbutton.button);
			bool axisType = false;
			if (button == GAMEPAD_BUTTON_DPAD_UP || button == GAMEPAD_BUTTON_DPAD_DOWN || button == GAMEPAD_BUTTON_DPAD_LEFT || button == GAMEPAD_BUTTON_DPAD_RIGHT)
			{
				for (uint32_t gamepadIndex = 0; gamepadIndex < MAX_GAMEPADS; gamepadIndex++)
				{
					if (gamepadManager->gamepads[gamepadIndex].joystickInstanceID == joystickInstanceID)
					{
						axisType = gamepadManager->gamepads[gamepadIndex].dpadAxis;
						break;
					}
				}
			}

			_addButtonEventIfNeeded((GamepadIndex)joystickInstanceID, button, axisType ? GAMEPAD_ELEMENT_MAPPING_TYPE_AXIS : GAMEPAD_ELEMENT_MAPPING_TYPE_BUTTON, sdlEvent->common.timestamp, (sdlEvent->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN), NULL, eventsBuffer, eventCount);
			
			break;
		}
		case SDL_EVENT_GAMEPAD_AXIS_MOTION:
		{
			Uint8 sdlAxis = sdlEvent->gaxis.axis;
			if (sdlAxis == SDL_GAMEPAD_AXIS_LEFTX || sdlAxis == SDL_GAMEPAD_AXIS_LEFTY)
			{
				SDL_JoystickID joystickInstanceID = sdlEvent->gbutton.which;
				for (uint32_t gamepadIndex = 0; gamepadIndex < MAX_GAMEPADS; gamepadIndex++)
				{
					Gamepad* gamepad = &gamepadManager->gamepads[gamepadIndex];
					if (gamepad->joystickInstanceID == joystickInstanceID)
					{
						Sint16 sdlAxisValue = sdlEvent->gaxis.value;

						if (sdlAxis == SDL_GAMEPAD_AXIS_LEFTX)
						{
							uint64_t timestamp = sdlEvent->common.timestamp;

							_addButtonEventIfNeeded((GamepadIndex)gamepad->joystickInstanceID, GAMEPAD_BUTTON_DPAD_RIGHT, GAMEPAD_ELEMENT_MAPPING_TYPE_AXIS, timestamp, (sdlAxisValue >= AXIS_THRESHOLD), &gamepad->lastRightAxisState, eventsBuffer, eventCount);

							_addButtonEventIfNeeded((GamepadIndex)gamepad->joystickInstanceID, GAMEPAD_BUTTON_DPAD_LEFT, GAMEPAD_ELEMENT_MAPPING_TYPE_AXIS, timestamp, (sdlAxisValue <= -AXIS_THRESHOLD), &gamepad->lastLeftAxisState, eventsBuffer, eventCount);
						}
						else if (sdlAxis == SDL_GAMEPAD_AXIS_LEFTY)
						{
							uint64_t timestamp = sdlEvent->common.timestamp;

							_addButtonEventIfNeeded((GamepadIndex)gamepad->joystickInstanceID, GAMEPAD_BUTTON_DPAD_DOWN, GAMEPAD_ELEMENT_MAPPING_TYPE_AXIS, timestamp, (sdlAxisValue >= AXIS_THRESHOLD), &gamepad->lastDownAxisState, eventsBuffer, eventCount);

							_addButtonEventIfNeeded((GamepadIndex)gamepad->joystickInstanceID, GAMEPAD_BUTTON_DPAD_UP, GAMEPAD_ELEMENT_MAPPING_TYPE_AXIS, timestamp, (sdlAxisValue <= -AXIS_THRESHOLD), &gamepad->lastUpAxisState, eventsBuffer, eventCount);
						}

						break;
					}
				}
			}
			break;
		}
		default:
			break;
	}
	
	return eventsBuffer;
}

const char *gamepadName(GamepadManager *gamepadManager, GamepadIndex index)
{
	SDL_Gamepad *controller = SDL_GetGamepadFromID(index);
	return SDL_GetGamepadName(controller);
}

uint8_t gamepadRank(GamepadManager *gamepadManager, GamepadIndex index)
{
	SDL_Gamepad* controller = SDL_GetGamepadFromID(index);

	switch (SDL_GetGamepadType(controller))
	{
	case SDL_GAMEPAD_TYPE_STANDARD:
	case SDL_GAMEPAD_TYPE_UNKNOWN:
	case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
	case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
	case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
	case SDL_GAMEPAD_TYPE_COUNT:
		return LOWEST_GAMEPAD_RANK;
	case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
	case SDL_GAMEPAD_TYPE_PS3:
	case SDL_GAMEPAD_TYPE_PS4:
	case SDL_GAMEPAD_TYPE_PS5:
	case SDL_GAMEPAD_TYPE_GAMECUBE:
		return 2;
	case SDL_GAMEPAD_TYPE_XBOX360:
	case SDL_GAMEPAD_TYPE_XBOXONE: {
		int playerIndex = SDL_GetGamepadPlayerIndex(controller);
		int rankIndex;
		if (playerIndex >= 0 && playerIndex < 4)
		{
			rankIndex = 3 - playerIndex;
		}
		else
		{
			rankIndex = -1;
		}
		return 4 + rankIndex;
	}
	}
	return 0;
}

void setPlayerIndex(GamepadManager *gamepadManager, GamepadIndex index, int64_t playerIndex)
{
	SDL_Gamepad* gamepad = SDL_GetGamepadFromID(index);
	SDL_SetGamepadPlayerIndex(gamepad, (int)playerIndex);
}
