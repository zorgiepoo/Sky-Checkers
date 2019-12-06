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
#import "platforms.h"

#ifdef IOS_DEVICE
#define GC_NAME(x) x
#else
#define GC_NAME(x) GC_##x
#endif

typedef struct GC_NAME(_Gamepad)
{
	void *controller;
	GamepadState lastStates[GAMEPAD_BUTTON_MAX];
	char name[GAMEPAD_NAME_SIZE];
	GamepadIndex index;
} GC_NAME(Gamepad);

struct GC_NAME(_GamepadManager)
{
	GC_NAME(Gamepad) gamepads[MAX_GAMEPADS];
	GamepadEvent eventsBuffer[GAMEPAD_EVENT_BUFFER_CAPACITY];
	GamepadCallback addedCallback;
	GamepadCallback removalCallback;
	void *context;
	GamepadIndex nextGamepadIndex;
};

struct GC_NAME(_GamepadManager) *GC_NAME(initGamepadManager)(const char *databasePath, GamepadCallback addedCallback, GamepadCallback removalCallback, void *context);

GamepadEvent *GC_NAME(pollGamepadEvents)(struct GC_NAME(_GamepadManager) *gamepadManager, const void *systemEvent, uint16_t *eventCount);

const char *GC_NAME(gamepadName)(struct GC_NAME(_GamepadManager) *gamepadManager, GamepadIndex index);
