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

#ifdef __cplusplus
extern "C" {
#endif

#include "platforms.h"
#include <stdint.h>
#include <stdbool.h>

#if PLATFORM_OSX
typedef enum
{
	ZG_KEYCODE_B = 11,
	ZG_KEYCODE_C = 8,
	ZG_KEYCODE_F = 3,
	ZG_KEYCODE_V = 9,
	ZG_KEYCODE_G = 5,
	ZG_KEYCODE_L = 37,
	ZG_KEYCODE_J = 38,
	ZG_KEYCODE_I = 34,
	ZG_KEYCODE_K = 40,
	ZG_KEYCODE_M = 46,
	ZG_KEYCODE_D = 2,
	ZG_KEYCODE_A = 0,
	ZG_KEYCODE_W = 13,
	ZG_KEYCODE_S = 1,
	ZG_KEYCODE_Z = 6,
	ZG_KEYCODE_RIGHT = 124,
	ZG_KEYCODE_LEFT = 123,
	ZG_KEYCODE_UP = 126,
	ZG_KEYCODE_DOWN = 125,
	ZG_KEYCODE_SPACE = 49,
	ZG_KEYCODE_ESCAPE = 53,
	ZG_KEYCODE_GRAVE = 50,
	ZG_KEYCODE_BACKSPACE = 51
} ZGConstantKeyCode;
#elif PLATFORM_WINDOWS
typedef enum
{
	ZG_KEYCODE_B = 0x42,
	ZG_KEYCODE_C = 0x43,
	ZG_KEYCODE_F = 0x46,
	ZG_KEYCODE_V = 0x56,
	ZG_KEYCODE_G = 0x47,
	ZG_KEYCODE_L = 0x4C,
	ZG_KEYCODE_J = 0x4A,
	ZG_KEYCODE_I = 0x49,
	ZG_KEYCODE_K = 0x4B,
	ZG_KEYCODE_M = 0x4D,
	ZG_KEYCODE_D = 0x44,
	ZG_KEYCODE_A = 0x41,
	ZG_KEYCODE_W = 0x57,
	ZG_KEYCODE_S = 0x53,
	ZG_KEYCODE_Z = 0x5A,
	ZG_KEYCODE_RIGHT = 0x27,
	ZG_KEYCODE_LEFT = 0x25,
	ZG_KEYCODE_UP = 0x26,
	ZG_KEYCODE_DOWN = 0x28,
	ZG_KEYCODE_SPACE = 0x20,
	ZG_KEYCODE_ESCAPE = 0x1B,
	ZG_KEYCODE_GRAVE = 0xC0,
	ZG_KEYCODE_BACKSPACE = 0x08
} ZGConstantKeyCode;
#else
#include <SDL2/SDL.h>
typedef enum
{
	ZG_KEYCODE_B = SDL_SCANCODE_B,
	ZG_KEYCODE_C = SDL_SCANCODE_C,
	ZG_KEYCODE_F = SDL_SCANCODE_F,
	ZG_KEYCODE_V = SDL_SCANCODE_V,
	ZG_KEYCODE_G = SDL_SCANCODE_G,
	ZG_KEYCODE_L = SDL_SCANCODE_L,
	ZG_KEYCODE_J = SDL_SCANCODE_J,
	ZG_KEYCODE_I = SDL_SCANCODE_I,
	ZG_KEYCODE_K = SDL_SCANCODE_K,
	ZG_KEYCODE_M = SDL_SCANCODE_M,
	ZG_KEYCODE_D = SDL_SCANCODE_D,
	ZG_KEYCODE_A = SDL_SCANCODE_A,
	ZG_KEYCODE_W = SDL_SCANCODE_W,
	ZG_KEYCODE_S = SDL_SCANCODE_S,
	ZG_KEYCODE_Z = SDL_SCANCODE_Z,
	ZG_KEYCODE_RIGHT = SDL_SCANCODE_RIGHT,
	ZG_KEYCODE_LEFT = SDL_SCANCODE_LEFT,
	ZG_KEYCODE_UP = SDL_SCANCODE_UP,
	ZG_KEYCODE_DOWN = SDL_SCANCODE_DOWN,
	ZG_KEYCODE_SPACE = SDL_SCANCODE_SPACE,
	ZG_KEYCODE_ESCAPE = SDL_SCANCODE_ESCAPE,
	ZG_KEYCODE_GRAVE = SDL_SCANCODE_GRAVE,
	ZG_KEYCODE_BACKSPACE = SDL_SCANCODE_BACKSPACE
} ZGConstantKeyCode;
#endif

typedef enum
{
	ZGKeyboardEventTypeKeyDown,
	ZGKeyboardEventTypeKeyUp,
	ZGKeyboardEventTypeTextInput
} ZGKeyboardEventType;

typedef struct
{
	union
	{
		struct
		{
			uint64_t keyModifier;
			uint64_t timestamp;
			uint16_t keyCode;
		};
		char text[32];
	};
	ZGKeyboardEventType type;
} ZGKeyboardEvent;

bool ZGTestReturnKeyCode(uint16_t keyCode);
bool ZGTestMetaModifier(uint64_t modifier);

const char *ZGGetKeyCodeName(uint16_t keyCode);

char *ZGGetClipboardText(void);
void ZGFreeClipboardText(char *clipboardText);

#ifdef __cplusplus
}
#endif
