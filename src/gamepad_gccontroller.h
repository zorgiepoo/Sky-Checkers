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

#import "gamepad.h"
#import "platforms.h"
#import <stdbool.h>

#if PLATFORM_IOS
#define GC_KEYBOARD 1
#else
#define GC_KEYBOARD 0
#endif

typedef struct _Gamepad
{
	void *controller;
#if GC_KEYBOARD
	void *keyboard;
#endif
	GamepadState lastStates[GAMEPAD_BUTTON_MAX];
	char name[GAMEPAD_NAME_SIZE];
	GamepadIndex index;
	uint8_t rank;
} Gamepad;

struct _GamepadManager
{
	Gamepad gamepads[MAX_GAMEPADS];
	GamepadEvent eventsBuffer[GAMEPAD_EVENT_BUFFER_CAPACITY];
	GamepadCallback addedCallback;
	GamepadCallback removalCallback;
	void *context;
	GamepadIndex nextGamepadIndex;
};

struct _GamepadManager *initGamepadManager(const char *databasePath, GamepadCallback addedCallback, GamepadCallback removalCallback, void *context);

GamepadEvent *pollGamepadEvents(struct _GamepadManager *gamepadManager, const void *systemEvent, uint16_t *eventCount);

const char *gamepadName(struct _GamepadManager *gamepadManager, GamepadIndex index);

uint8_t gamepadRank(struct _GamepadManager *gamepadManager, GamepadIndex index);

void setPlayerIndex(struct _GamepadManager *gamepadManager, GamepadIndex index, int64_t playerIndex);
