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

#import "gamepad.h"
#import "time.h"
#import <Foundation/Foundation.h>
#import <GameController/GameController.h>

typedef struct _Gamepad
{
	void *controller;
	GamepadState lastStates[GAMEPAD_BUTTON_MAX];
	char name[GAMEPAD_NAME_SIZE];
	GamepadIndex index;
} Gamepad;

struct _GamepadManager
{
	Gamepad gamepads[MAX_GAMEPADS];
	GamepadEvent eventsBuffer[GAMEPAD_EVENT_BUFFER_CAPACITY];
	GamepadCallback addedCallback;
	GamepadCallback removalCallback;
	GamepadIndex nextGamepadIndex;
};

static void _removeController(GamepadManager *gamepadManager, GCController *controller)
{
	for (uint16_t gamepadIndex = 0; gamepadIndex < MAX_GAMEPADS; gamepadIndex++)
	{
		Gamepad *gamepad = &gamepadManager->gamepads[gamepadIndex];
		if (gamepad->controller == (__bridge void *)(controller))
		{
			if (gamepadManager->removalCallback != NULL)
			{
				gamepadManager->removalCallback(gamepad->index);
			}
			
			CFRelease(gamepad->controller);
			memset(gamepad, 0, sizeof(*gamepad));
			break;
		}
	}
}

static void _addController(GamepadManager *gamepadManager, GCController *controller)
{
	uint16_t availableGamepadIndex = MAX_GAMEPADS;
	for (uint16_t gamepadIndex = 0; gamepadIndex < MAX_GAMEPADS; gamepadIndex++)
	{
		Gamepad *gamepad = &gamepadManager->gamepads[gamepadIndex];
		if (gamepad->controller == (__bridge void *)(controller))
		{
			return;
		}
		else if (gamepad->controller == nil && availableGamepadIndex == MAX_GAMEPADS)
		{
			availableGamepadIndex = gamepadIndex;
		}
	}
	
	if (availableGamepadIndex < MAX_GAMEPADS)
	{
		Gamepad *gamepad = &gamepadManager->gamepads[availableGamepadIndex];
		gamepad->controller = (void *)CFBridgingRetain(controller);
		NSString *vendorName = controller.vendorName;
		if (vendorName != nil)
		{
			const char *vendorNameUTF8 = vendorName.UTF8String;
			if (vendorNameUTF8 != NULL)
			{
				strncpy(gamepad->name, vendorNameUTF8, sizeof(gamepad->name) - 1);
			}
		}
		gamepad->index = gamepadManager->nextGamepadIndex;
		gamepadManager->nextGamepadIndex++;
		
		if (gamepadManager->addedCallback != NULL)
		{
			gamepadManager->addedCallback(gamepad->index);
		}
	}
}

GamepadManager *initGamepadManager(const char *databasePath, GamepadCallback addedCallback, GamepadCallback removalCallback)
{
	GamepadManager *gamepadManager = calloc(1, sizeof(*gamepadManager));
	gamepadManager->addedCallback = addedCallback;
	gamepadManager->removalCallback = removalCallback;
	
	for (GCController *controller in [GCController controllers])
	{
		_addController(gamepadManager, controller);
	}
	
	[[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidConnectNotification object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
		_addController(gamepadManager, [note object]);
	}];
	
	[[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidDisconnectNotification object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
		_removeController(gamepadManager, [note object]);
	}];
	
	return gamepadManager;
}

static void _addButtonEventIfNeeded(Gamepad *gamepad, GamepadButton button, GCControllerButtonInput *buttonInput, GamepadEvent *eventsBuffer, uint16_t *eventIndex)
{
	BOOL pressed = buttonInput.pressed;
	if ((GamepadState)pressed != gamepad->lastStates[button])
	{
		gamepad->lastStates[button] = pressed;
		
		GamepadState state = pressed ? GAMEPAD_STATE_PRESSED : GAMEPAD_STATE_RELEASED;
		
		GamepadEvent event = {.button = button, .index = gamepad->index, .state = state, .mappingType = GAMEPAD_ELEMENT_MAPPING_TYPE_BUTTON, .ticks = ZGGetTicks()};
		
		eventsBuffer[*eventIndex] = event;
		(*eventIndex)++;
	}
}

GamepadEvent *pollGamepadEvents(GamepadManager *gamepadManager, const void *systemEvent, uint16_t *eventCount)
{
	if (systemEvent != NULL)
	{
		*eventCount = 0;
		return NULL;
	}
	
	uint16_t eventIndex = 0;
	for (uint16_t gamepadIndex = 0; gamepadIndex < MAX_GAMEPADS; gamepadIndex++)
	{
		Gamepad *gamepad = &gamepadManager->gamepads[gamepadIndex];
		GCController *controller = (__bridge GCController *)(gamepad->controller);
		if (controller != nil)
		{
			GCExtendedGamepad *extendedGamepad = controller.extendedGamepad;
			if (extendedGamepad != nil)
			{
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_A, extendedGamepad.buttonA, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_B, extendedGamepad.buttonB, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_X, extendedGamepad.buttonX, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_Y, extendedGamepad.buttonY, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_LEFTSHOULDER, extendedGamepad.leftShoulder, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_RIGHTSHOULDER, extendedGamepad.rightShoulder, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_LEFTTRIGGER, extendedGamepad.leftTrigger, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_RIGHTTRIGGER, extendedGamepad.rightTrigger, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_START, extendedGamepad.buttonMenu, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_BACK, extendedGamepad.buttonOptions, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_UP, extendedGamepad.dpad.up, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_DOWN, extendedGamepad.dpad.down, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_LEFT, extendedGamepad.dpad.left, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_RIGHT, extendedGamepad.dpad.right, gamepadManager->eventsBuffer, &eventIndex);
			}
			else
			{
				GCMicroGamepad *microGamepad = controller.microGamepad;
				if (microGamepad != nil)
				{
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_A, microGamepad.buttonA, gamepadManager->eventsBuffer, &eventIndex);
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_B, microGamepad.buttonX, gamepadManager->eventsBuffer, &eventIndex);
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_START, microGamepad.buttonMenu, gamepadManager->eventsBuffer, &eventIndex);
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_UP, microGamepad.dpad.up, gamepadManager->eventsBuffer, &eventIndex);
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_DOWN, microGamepad.dpad.down, gamepadManager->eventsBuffer, &eventIndex);
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_LEFT, microGamepad.dpad.left, gamepadManager->eventsBuffer, &eventIndex);
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_RIGHT, microGamepad.dpad.right, gamepadManager->eventsBuffer, &eventIndex);
				}
			}
		}
	}
	
	*eventCount = eventIndex;
	return gamepadManager->eventsBuffer;
}

const char *gamepadName(GamepadManager *gamepadManager, GamepadIndex index)
{
	for (uint16_t gamepadIndex = 0; gamepadIndex < MAX_GAMEPADS; gamepadIndex++)
	{
		Gamepad *gamepad = &gamepadManager->gamepads[gamepadIndex];
		if (gamepad->index == index)
		{
			return gamepad->name;
		}
	}
	return NULL;
}
