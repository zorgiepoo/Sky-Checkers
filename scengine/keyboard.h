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
