/*
 MIT License

 Copyright (c) 2024 Mayur Pawashe

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

#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "platforms.h"

#define MAX_GAMEPADS 8
#define GAMEPAD_NAME_SIZE 256

#define LOWEST_GAMEPAD_RANK 1
#define AXIS_THRESHOLD_PERCENT 0.6f

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

#if PLATFORM_WINDOWS
void pollNewGamepads(GamepadManager* gamepadManager);
#endif
