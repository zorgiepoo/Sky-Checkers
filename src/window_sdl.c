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

#include <SDL2/SDL.h>

#include "window.h"
#include "zgtime.h"

// for setting window icon
#include <unistd.h>
#include <assert.h>
#include "texture.h"

bool _ZGSetWindowFullscreen(ZGWindow *window, bool enabled, const char **errorString);

typedef struct
{
	SDL_Window *window;
	bool *fullscreenFlag;
	
	void (*windowEventHandler)(ZGWindowEvent, void *);
	void *windowEventHandlerContext;
	
	void (*keyboardEventHandler)(ZGKeyboardEvent, void *);
	void *keyboardEventHandlerContext;
} WindowController;

ZGWindow *ZGCreateWindow(const char *windowTitle, int32_t windowWidth, int32_t windowHeight, bool *fullscreenFlag)
{
	Uint32 videoFlags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;
	if (fullscreenFlag != NULL && *fullscreenFlag)
	{
		videoFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}

	SDL_ShowCursor(SDL_DISABLE);
	
	WindowController *windowController = calloc(1, sizeof(*windowController));
	windowController->fullscreenFlag =  fullscreenFlag;
	windowController->window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, videoFlags);

	// Set the window icon if available
	const char *iconPath = "Data/Textures/sc_icon.bmp";
	if (access(iconPath, F_OK) != -1)
	{
		TextureData iconData = loadTextureData(iconPath);
		SDL_Surface *iconSurface = iconData.context;
		assert(iconSurface != NULL);

		// Filter out bright background and make transparent
		const int iconThresholdFilter = 70;
		for (uint32_t pixelIndex = 0; pixelIndex < (uint32_t)(iconData.width * iconData.height); pixelIndex++)
		{
			uint8_t *colorData = &iconData.pixelData[pixelIndex * 4];
			if (colorData[0] >= iconThresholdFilter && colorData[1] >= iconThresholdFilter && colorData[2] >= iconThresholdFilter)
			{
				colorData[3] = 0;
			}
		}
	
		SDL_SetWindowIcon(windowController->window, iconSurface);
	
		freeTextureData(iconData);
	}
	return windowController;
}

void ZGDestroyWindow(ZGWindow *windowRef)
{
	WindowController *windowController = (WindowController *)windowRef;
	SDL_DestroyWindow(windowController->window);
	
	windowController->window = NULL;
	windowController->windowEventHandler = NULL;
	windowController->windowEventHandlerContext = NULL;
	windowController->keyboardEventHandler = NULL;
	windowController->keyboardEventHandlerContext = NULL;
	
	free(windowController);
}

bool ZGWindowHasFocus(ZGWindow *windowRef)
{
	WindowController *windowController = (WindowController *)windowRef;
	return (SDL_GetWindowFlags(windowController->window) & SDL_WINDOW_INPUT_FOCUS) != 0;
}

void ZGSetWindowEventHandler(ZGWindow *windowRef, void *context, void (*windowEventHandler)(ZGWindowEvent, void *))
{
	WindowController *windowController = (WindowController *)windowRef;
	windowController->windowEventHandler = windowEventHandler;
	windowController->windowEventHandlerContext = context;
}

void ZGSetKeyboardEventHandler(ZGWindow *windowRef, void *context, void (*keyboardEventHandler)(ZGKeyboardEvent, void *))
{
	WindowController *windowController = (WindowController *)windowRef;
	windowController->keyboardEventHandler = keyboardEventHandler;
	windowController->keyboardEventHandlerContext = context;
}

void ZGPollWindowAndInputEvents(ZGWindow *windowRef, const void *systemEvent)
{
	WindowController *windowController = (WindowController *)windowRef;
	if (systemEvent == NULL) return;
	
	const SDL_Event *sdlEvent = systemEvent;
	switch (sdlEvent->type)
	{
		case SDL_WINDOWEVENT:
		{
			if (windowController->windowEventHandler == NULL) return;
			
			switch (sdlEvent->window.event)
			{
				case SDL_WINDOWEVENT_FOCUS_LOST:
				{
					ZGWindowEvent event;
					event.type = ZGWindowEventTypeFocusLost;

					windowController->windowEventHandler(event, windowController->windowEventHandlerContext);
				} break;
				case SDL_WINDOWEVENT_FOCUS_GAINED:
				{
					ZGWindowEvent event;
					event.type = ZGWindowEventTypeFocusGained;

					windowController->windowEventHandler(event, windowController->windowEventHandlerContext);
				} break;
				case SDL_WINDOWEVENT_HIDDEN:
				{
					ZGWindowEvent event;
					event.type = ZGWindowEventTypeHidden;

					windowController->windowEventHandler(event, windowController->windowEventHandlerContext);
				} break;
				case SDL_WINDOWEVENT_SHOWN:
				{
					ZGWindowEvent event;
					event.type = ZGWindowEventTypeShown;
					
					windowController->windowEventHandler(event, windowController->windowEventHandlerContext);
				} break;
				case SDL_WINDOWEVENT_RESIZED:
				{
					ZGWindowEvent event;
					event.type = ZGWindowEventTypeResize;
					event.width = sdlEvent->window.data1;
					event.height = sdlEvent->window.data2;
					
					windowController->windowEventHandler(event, windowController->windowEventHandlerContext);
				} break;
			}
		} break;
		case SDL_KEYDOWN:
		{
			if (ZGTestReturnKeyCode(sdlEvent->key.keysym.scancode) && ((sdlEvent->key.keysym.mod & (KMOD_LALT | KMOD_RALT)) != 0))
			{
				const char *fullscreenErrorString = NULL;
				if (!ZGWindowIsFullscreen(windowController))
				{
					if (!_ZGSetWindowFullscreen(windowController, true, &fullscreenErrorString))
					{
						fprintf(stderr, "Failed to set fullscreen because: %s\n", fullscreenErrorString);
					}
					else
					{
						*windowController->fullscreenFlag = true;
					}
				}
				else
				{
					if (!_ZGSetWindowFullscreen(windowController, false, &fullscreenErrorString))
					{
						fprintf(stderr, "Failed to escape fullscreen because: %s\n", fullscreenErrorString);
					}
					else
					{
						*windowController->fullscreenFlag = false;
					}
				}
			}
			else if (windowController->keyboardEventHandler != NULL)
			{
				ZGKeyboardEvent event;
				event.type = ZGKeyboardEventTypeKeyDown;
				event.keyCode = sdlEvent->key.keysym.scancode;
				event.keyModifier = sdlEvent->key.keysym.mod;
				event.timestamp = CONVERT_MS_TO_NS(sdlEvent->key.timestamp);
				
				windowController->keyboardEventHandler(event, windowController->keyboardEventHandlerContext);
			}
		} break;
		case SDL_KEYUP:
		{
			if (windowController->keyboardEventHandler != NULL)
			{
				ZGKeyboardEvent event;
				event.type = ZGKeyboardEventTypeKeyUp;
				event.keyCode = sdlEvent->key.keysym.scancode;
				event.keyModifier = sdlEvent->key.keysym.mod;
				event.timestamp = CONVERT_MS_TO_NS(sdlEvent->key.timestamp);
				
				windowController->keyboardEventHandler(event, windowController->keyboardEventHandlerContext);
			}
		} break;
		case SDL_TEXTINPUT:
		{
			if (windowController->keyboardEventHandler != NULL)
			{
				ZGKeyboardEvent event;
				event.keyCode = 0;
				event.keyModifier = 0;
				event.timestamp = 0;
				event.type = ZGKeyboardEventTypeTextInput;
				strncpy(event.text, sdlEvent->text.text, sizeof(event.text));
				
				windowController->keyboardEventHandler(event, windowController->keyboardEventHandlerContext);
			}
		} break;
	}
}

bool ZGWindowIsFullscreen(ZGWindow *windowRef)
{
	WindowController *windowController = (WindowController *)windowRef;
	return (SDL_GetWindowFlags(windowController->window) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) != 0;
}

bool _ZGSetWindowFullscreen(ZGWindow *windowRef, bool enabled, const char **errorString)
{
	WindowController *windowController = (WindowController *)windowRef;
	bool result = SDL_SetWindowFullscreen(windowController->window, enabled ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) == 0;
	if (!result && errorString != NULL)
	{
		*errorString = SDL_GetError();
	}
	return result;
}

void ZGGetWindowSize(ZGWindow *windowRef, int32_t *width, int32_t *height)
{
	WindowController *windowController = (WindowController *)windowRef;
	SDL_GetWindowSize(windowController->window, width, height);
}

void *ZGWindowHandle(ZGWindow *windowRef)
{
	WindowController *windowController = (WindowController *)windowRef;
	return windowController->window;
}

