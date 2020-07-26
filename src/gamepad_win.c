/*
* Copyright 2020 Mayur Pawashe
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
#include "zgtime.h"

#include <Windows.h>
#include <Xinput.h>
#include <stdbool.h>

typedef struct
{
	bool connected;
	XINPUT_STATE lastState;
} Gamepad;

struct _GamepadManager
{
	GamepadCallback addedCallback;
	GamepadCallback removalCallback;
	void* context;
	GamepadEvent eventsBuffer[XUSER_MAX_COUNT * GAMEPAD_BUTTON_MAX];
	Gamepad gamepads[XUSER_MAX_COUNT];
};

GamepadManager* initGamepadManager(const char* databasePath, GamepadCallback addedCallback, GamepadCallback removalCallback, void* context)
{
	GamepadManager* gamepadManager = calloc(1, sizeof(*gamepadManager));

	gamepadManager->addedCallback = addedCallback;
	gamepadManager->removalCallback = removalCallback;
	gamepadManager->context = context;

	// Scan existing gamepads
	for (DWORD instanceID = 0; instanceID < sizeof(gamepadManager->gamepads) / sizeof(*gamepadManager->gamepads); instanceID++)
	{
		XINPUT_STATE state;
		if (XInputGetState(instanceID, &state) == ERROR_SUCCESS)
		{
			gamepadManager->gamepads[instanceID].connected = true;
			gamepadManager->addedCallback(instanceID, gamepadManager->context);
		}
		else
		{
			gamepadManager->gamepads[instanceID].connected = false;
		}
	}

	return gamepadManager;
}

static void _addEventIfChanged(DWORD instanceID, bool wasHeldDown, bool isHeldDown, GamepadButton gamepadButton, GamepadElementMappingType elementMappingType, GamepadEvent* events, uint16_t* currentEventIndex)
{
	if (wasHeldDown != isHeldDown)
	{
		GamepadState gamepadState = (isHeldDown ? GAMEPAD_STATE_PRESSED : GAMEPAD_STATE_RELEASED);
		GamepadEvent event = { .ticks = ZGGetNanoTicks(), .state = gamepadState, .button = gamepadButton, .mappingType = elementMappingType, .index = (GamepadIndex)instanceID };

		events[*currentEventIndex] = event;
		(*currentEventIndex)++;
	}
}

static void _addButtonEventIfChanged(DWORD instanceID, WORD lastButtons, WORD newButtons, GamepadButton gamepadButton, WORD mask, GamepadEvent* events, uint16_t *currentEventIndex)
{
	bool wasHeldDown = (lastButtons & mask) != 0;
	bool isHeldDown = (newButtons & mask) != 0;

	_addEventIfChanged(instanceID, wasHeldDown, isHeldDown, gamepadButton, GAMEPAD_ELEMENT_MAPPING_TYPE_BUTTON, events, currentEventIndex);
}

static void _addAxisEventIfChanged(DWORD instanceID, SHORT lastAxisValue, SHORT newAxisValue, SHORT threshold, GamepadButton gamepadButton, bool positiveDirection, GamepadEvent* events, uint16_t* currentEventIndex)
{
	bool wasHeldDown = positiveDirection ? (lastAxisValue >= threshold) : (lastAxisValue <= -threshold);
	bool isHeldDown = positiveDirection ? (newAxisValue >= threshold) : (newAxisValue <= -threshold);

	_addEventIfChanged(instanceID, wasHeldDown, isHeldDown, gamepadButton, GAMEPAD_ELEMENT_MAPPING_TYPE_AXIS, events, currentEventIndex);
}

GamepadEvent* pollGamepadEvents(GamepadManager* gamepadManager, const void* systemEvent, uint16_t* eventCount)
{
	if (systemEvent != NULL)
	{
		*eventCount = 0;
		return NULL;
	}

	GamepadEvent* eventsBuffer = gamepadManager->eventsBuffer;

	uint16_t numberOfEvents = 0;
	for (DWORD instanceID = 0; instanceID < sizeof(gamepadManager->gamepads) / sizeof(*gamepadManager->gamepads); instanceID++)
	{
		if (gamepadManager->gamepads[instanceID].connected)
		{
			XINPUT_STATE newState;
			DWORD stateResult = XInputGetState(instanceID, &newState);
			if (stateResult == ERROR_DEVICE_NOT_CONNECTED)
			{
				gamepadManager->removalCallback(instanceID, gamepadManager->context);
				memset(&gamepadManager->gamepads[instanceID], 0, sizeof(gamepadManager->gamepads[instanceID]));
			}
			else if (stateResult == ERROR_SUCCESS && newState.dwPacketNumber > gamepadManager->gamepads[instanceID].lastState.dwPacketNumber)
			{
				WORD lastButtons = gamepadManager->gamepads[instanceID].lastState.Gamepad.wButtons;
				WORD newButtons = newState.Gamepad.wButtons;

				_addButtonEventIfChanged(instanceID, lastButtons, newButtons, GAMEPAD_BUTTON_DPAD_UP, XINPUT_GAMEPAD_DPAD_UP, eventsBuffer, &numberOfEvents);
				_addButtonEventIfChanged(instanceID, lastButtons, newButtons, GAMEPAD_BUTTON_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_DOWN, eventsBuffer, &numberOfEvents);
				_addButtonEventIfChanged(instanceID, lastButtons, newButtons, GAMEPAD_BUTTON_DPAD_RIGHT, XINPUT_GAMEPAD_DPAD_RIGHT, eventsBuffer, &numberOfEvents);
				_addButtonEventIfChanged(instanceID, lastButtons, newButtons, GAMEPAD_BUTTON_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_LEFT, eventsBuffer, &numberOfEvents);
				_addButtonEventIfChanged(instanceID, lastButtons, newButtons, GAMEPAD_BUTTON_A, XINPUT_GAMEPAD_A, eventsBuffer, &numberOfEvents);
				_addButtonEventIfChanged(instanceID, lastButtons, newButtons, GAMEPAD_BUTTON_B, XINPUT_GAMEPAD_B, eventsBuffer, &numberOfEvents);
				_addButtonEventIfChanged(instanceID, lastButtons, newButtons, GAMEPAD_BUTTON_Y, XINPUT_GAMEPAD_Y, eventsBuffer, &numberOfEvents);
				_addButtonEventIfChanged(instanceID, lastButtons, newButtons, GAMEPAD_BUTTON_X, XINPUT_GAMEPAD_X, eventsBuffer, &numberOfEvents);
				_addButtonEventIfChanged(instanceID, lastButtons, newButtons, GAMEPAD_BUTTON_START, XINPUT_GAMEPAD_START, eventsBuffer, &numberOfEvents);
				_addButtonEventIfChanged(instanceID, lastButtons, newButtons, GAMEPAD_BUTTON_BACK, XINPUT_GAMEPAD_BACK, eventsBuffer, &numberOfEvents);
				_addButtonEventIfChanged(instanceID, lastButtons, newButtons, GAMEPAD_BUTTON_RIGHTSHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER, eventsBuffer, &numberOfEvents);
				_addButtonEventIfChanged(instanceID, lastButtons, newButtons, GAMEPAD_BUTTON_LEFTSHOULDER, XINPUT_GAMEPAD_LEFT_SHOULDER, eventsBuffer, &numberOfEvents);

				_addAxisEventIfChanged(instanceID, gamepadManager->gamepads[instanceID].lastState.Gamepad.sThumbLX, newState.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, GAMEPAD_BUTTON_DPAD_RIGHT, true, eventsBuffer, &numberOfEvents);
				_addAxisEventIfChanged(instanceID, gamepadManager->gamepads[instanceID].lastState.Gamepad.sThumbLX, newState.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, GAMEPAD_BUTTON_DPAD_LEFT, false, eventsBuffer, &numberOfEvents);
				_addAxisEventIfChanged(instanceID, gamepadManager->gamepads[instanceID].lastState.Gamepad.sThumbLY, newState.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, GAMEPAD_BUTTON_DPAD_UP, true, eventsBuffer, &numberOfEvents);
				_addAxisEventIfChanged(instanceID, gamepadManager->gamepads[instanceID].lastState.Gamepad.sThumbLY, newState.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, GAMEPAD_BUTTON_DPAD_DOWN, false, eventsBuffer, &numberOfEvents);

				gamepadManager->gamepads[instanceID].lastState = newState;
			}
		}
	}

	*eventCount = numberOfEvents;
	return eventsBuffer;
}

const char* gamepadName(GamepadManager* gamepadManager, GamepadIndex index)
{
	static char nameBuffer[8] = { 0 };
	snprintf(nameBuffer, sizeof(nameBuffer), "Xbox %d", (int)(index + 1));
	return nameBuffer;
}

uint8_t gamepadRank(GamepadManager* gamepadManager, GamepadIndex index)
{
	uint8_t maxGamepads = (uint8_t)(sizeof(gamepadManager->gamepads) / sizeof(*gamepadManager->gamepads));
	return LOWEST_GAMEPAD_RANK + 1 + ((maxGamepads - 1) - index);
}

void setPlayerIndex(GamepadManager* gamepadManager, GamepadIndex index, int64_t playerIndex)
{
}
