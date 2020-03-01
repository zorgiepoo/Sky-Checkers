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

#include "gamepad.h"
#include "sdl_include.h"
#include "zgtime.h"

#include <stdbool.h>
#include <string.h>

#define INVALID_JOYSTICK_INSTANCE_ID -1

typedef struct
{
	SDL_JoystickID joystickInstanceID;
	bool dpadAxis;
} Gamepad;

struct _GamepadManager
{
	GamepadCallback addedCallback;
	GamepadCallback removalCallback;
	void *context;
	GamepadEvent lastEvent;
	Gamepad gamepads[MAX_GAMEPADS];
};

static void _notifyGamepadAdded(GamepadManager *gamepadManager, Sint32 joystickIndex)
{
	if (SDL_IsGameController(joystickIndex))
	{
		SDL_GameController *controller = SDL_GameControllerOpen(joystickIndex);
		if (controller == NULL)
		{
			fprintf(stderr, "Failed to open gamecontroller %d: %s\n", joystickIndex, SDL_GetError());
		}
		else
		{
			SDL_JoystickID joystickInstanceID = SDL_JoystickGetDeviceInstanceID(joystickIndex);
			for (SDL_JoystickID gamepadIndex = 0; gamepadIndex < MAX_GAMEPADS; gamepadIndex++)
			{
				if (gamepadManager->gamepads[gamepadIndex].joystickInstanceID == INVALID_JOYSTICK_INSTANCE_ID)
				{
					gamepadManager->gamepads[gamepadIndex].joystickInstanceID = joystickInstanceID;
					gamepadManager->gamepads[gamepadIndex].dpadAxis = false;
					
					char *controllerMapping = SDL_GameControllerMapping(controller);
					if (controllerMapping != NULL)
					{
						const char *prefixString = "dpdown:";
						char *substring = strstr(controllerMapping, prefixString);
						if (substring != NULL)
						{
							char *codeString = substring + sizeof(prefixString);
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

GamepadManager *initGamepadManager(const char *databasePath, GamepadCallback addedCallback, GamepadCallback removalCallback, void *context)
{
	if (SDL_Init(SDL_INIT_GAMECONTROLLER) != 0)
	{
		fprintf(stderr, "Failed to initialize SDL gamecontroller: %s\n", SDL_GetError());
		return NULL;
	}
	
	if (SDL_GameControllerAddMappingsFromFile(databasePath) == -1)
	{
		fprintf(stderr, "Failed to add SDL game controller mappings: %s\n", SDL_GetError());
	}
	
	GamepadManager *gamepadManager = calloc(1, sizeof(*gamepadManager));
	for (SDL_JoystickID gamepadIndex = 0; gamepadIndex < MAX_GAMEPADS; gamepadIndex++)
	{
		gamepadManager->gamepads[gamepadIndex].joystickInstanceID = INVALID_JOYSTICK_INSTANCE_ID;
	}
	
	gamepadManager->addedCallback = addedCallback;
	gamepadManager->removalCallback = removalCallback;
	gamepadManager->context = context;
	
	// Scan already connected devices
	for (Sint32 joystickIndex = 0; joystickIndex < SDL_NumJoysticks(); joystickIndex++)
	{
		_notifyGamepadAdded(gamepadManager, joystickIndex);
	}
	
	return gamepadManager;
}

GamepadButton _gamepadButtonForSDLButton(Uint8 sdlButton)
{
	switch (sdlButton)
	{
		case SDL_CONTROLLER_BUTTON_A:
			return GAMEPAD_BUTTON_A;
		case SDL_CONTROLLER_BUTTON_B:
			return GAMEPAD_BUTTON_B;
		case SDL_CONTROLLER_BUTTON_X:
			return GAMEPAD_BUTTON_X;
		case SDL_CONTROLLER_BUTTON_Y:
			return GAMEPAD_BUTTON_Y;
		case SDL_CONTROLLER_BUTTON_BACK:
			return GAMEPAD_BUTTON_BACK;
		case SDL_CONTROLLER_BUTTON_START:
			return GAMEPAD_BUTTON_START;
		case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
			return GAMEPAD_BUTTON_LEFTSHOULDER;
		case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
			return GAMEPAD_BUTTON_RIGHTSHOULDER;
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
			return GAMEPAD_BUTTON_DPAD_UP;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			return GAMEPAD_BUTTON_DPAD_DOWN;
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			return GAMEPAD_BUTTON_DPAD_LEFT;
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
			return GAMEPAD_BUTTON_DPAD_RIGHT;
		case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
		case SDL_CONTROLLER_BUTTON_LEFTSTICK:
		case SDL_CONTROLLER_BUTTON_GUIDE:
		case (Uint8)SDL_CONTROLLER_BUTTON_INVALID:
		case SDL_CONTROLLER_BUTTON_MAX:
		default:
			return GAMEPAD_BUTTON_MAX;
	}
}

GamepadEvent *pollGamepadEvents(GamepadManager *gamepadManager, const void *systemEvent, uint16_t *eventCount)
{
	*eventCount = 0;
	
	if (systemEvent == NULL)
	{
		return NULL;
	}
	
	GamepadEvent *event = &gamepadManager->lastEvent;
	
	const SDL_Event *sdlEvent = systemEvent;
	
	switch (sdlEvent->type)
	{
		case SDL_CONTROLLERDEVICEADDED:
			_notifyGamepadAdded(gamepadManager, sdlEvent->cdevice.which);
			break;
		case SDL_CONTROLLERDEVICEREMOVED:
		{
			SDL_GameController *controller = SDL_GameControllerFromInstanceID(sdlEvent->cdevice.which);

			if (controller != NULL)
			{
				SDL_GameControllerClose(controller);
			}
			
			SDL_JoystickID joystickInstanceID = sdlEvent->cdevice.which;
			for (SDL_JoystickID gamepadIndex = 0; gamepadIndex < MAX_GAMEPADS; gamepadIndex++)
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
		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
		{
			SDL_JoystickID joystickInstanceID = sdlEvent->cbutton.which;
			GamepadButton button = _gamepadButtonForSDLButton(sdlEvent->cbutton.button);
			bool axisType = false;
			if (button == GAMEPAD_BUTTON_DPAD_UP || button == GAMEPAD_BUTTON_DPAD_DOWN || button == GAMEPAD_BUTTON_DPAD_LEFT || button == GAMEPAD_BUTTON_DPAD_RIGHT)
			{
				for (SDL_JoystickID gamepadIndex = 0; gamepadIndex < MAX_GAMEPADS; gamepadIndex++)
				{
					if (gamepadManager->gamepads[gamepadIndex].joystickInstanceID == joystickInstanceID)
					{
						axisType = gamepadManager->gamepads[gamepadIndex].dpadAxis;
						break;
					}
				}
			}
			
			event->button = button;
			event->state = (sdlEvent->type == SDL_CONTROLLERBUTTONDOWN ? GAMEPAD_STATE_PRESSED : GAMEPAD_STATE_RELEASED);
			event->mappingType = axisType ? GAMEPAD_ELEMENT_MAPPING_TYPE_AXIS : GAMEPAD_ELEMENT_MAPPING_TYPE_BUTTON;
			event->ticks = CONVERT_MS_TO_NS(sdlEvent->common.timestamp);
			event->index = (GamepadIndex)joystickInstanceID;

			*eventCount = 1;
			
			break;
		}
		default:
			break;
	}
	
	return event;
}

const char *gamepadName(GamepadManager *gamepadManager, GamepadIndex index)
{
	SDL_GameController *controller = SDL_GameControllerFromInstanceID(index);
	return SDL_GameControllerName(controller);
}

void setPlayerIndex(GamepadManager *gamepadManager, GamepadIndex index, int64_t playerIndex)
{
}
