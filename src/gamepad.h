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

#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "platforms.h"

#define MAX_GAMEPADS 8
#define GAMEPAD_NAME_SIZE 256

#define LOWEST_GAMEPAD_RANK 1

typedef enum
{
	GAMEPAD_BUTTON_A = 0,
	GAMEPAD_BUTTON_B,
	GAMEPAD_BUTTON_X,
	GAMEPAD_BUTTON_Y,
	GAMEPAD_BUTTON_BACK,
	GAMEPAD_BUTTON_START,
	GAMEPAD_BUTTON_LEFTSHOULDER,
	GAMEPAD_BUTTON_RIGHTSHOULDER,
	GAMEPAD_BUTTON_LEFTTRIGGER,
	GAMEPAD_BUTTON_RIGHTTRIGGER,
	GAMEPAD_BUTTON_DPAD_UP,
	GAMEPAD_BUTTON_DPAD_DOWN,
	GAMEPAD_BUTTON_DPAD_LEFT,
	GAMEPAD_BUTTON_DPAD_RIGHT,
	GAMEPAD_BUTTON_MAX
} GamepadButton;

#define GAMEPAD_EVENT_BUFFER_CAPACITY (MAX_GAMEPADS * GAMEPAD_BUTTON_MAX)

typedef enum
{
	GAMEPAD_STATE_RELEASED = 0,
	GAMEPAD_STATE_PRESSED
} GamepadState;

typedef enum
{
	GAMEPAD_ELEMENT_MAPPING_TYPE_INVALID = 0,
	GAMEPAD_ELEMENT_MAPPING_TYPE_BUTTON,
	GAMEPAD_ELEMENT_MAPPING_TYPE_AXIS,
	GAMEPAD_ELEMENT_MAPPING_TYPE_HAT
} GamepadElementMappingType;

struct _GamepadManager;
typedef struct _GamepadManager GamepadManager;

typedef uint32_t GamepadIndex;

#define INVALID_GAMEPAD_INDEX ((GamepadIndex)-1)
#define UNSET_PLAYER_INDEX (-1)

typedef struct
{
	uint64_t ticks;
	GamepadState state;
	GamepadButton button;
	GamepadElementMappingType mappingType;
	GamepadIndex index;
} GamepadEvent;

typedef void (*GamepadCallback)(GamepadIndex index, void *context);

GamepadManager *initGamepadManager(const char *databasePath, GamepadCallback addedCallback, GamepadCallback removalCallback, void *context);

GamepadEvent *pollGamepadEvents(GamepadManager *gamepadManager, const void *systemEvent, uint16_t *eventCount);

const char *gamepadName(GamepadManager *gamepadManager, GamepadIndex index);

uint8_t gamepadRank(GamepadManager *gamepadManager, GamepadIndex index);

void setPlayerIndex(GamepadManager *gamepadManager, GamepadIndex index, int64_t playerIndex);
