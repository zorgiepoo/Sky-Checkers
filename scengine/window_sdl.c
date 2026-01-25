/*
 MIT License

 Copyright (c) 2019 Mayur Pawashe

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

#include <SDL3/SDL.h>
#include <stdlib.h>

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
	Uint32 videoFlags = SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;

	SDL_Window *window = SDL_CreateWindow(windowTitle, windowWidth, windowHeight, videoFlags);
	if (window == NULL)
	{
		fprintf(stderr, "Failed to create SDL window: %s\n", SDL_GetError());
		return NULL;
	}

	SDL_HideCursor();
	
	WindowController *windowController = calloc(1, sizeof(*windowController));
	windowController->fullscreenFlag =  fullscreenFlag;
	windowController->window = window;

	SDL_SetWindowFullscreen(windowController->window, (fullscreenFlag != NULL && *fullscreenFlag));

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

void ZGSetAcceptingTextInput(ZGWindow *windowRef, bool acceptTextInput)
{
	WindowController *windowController = (WindowController *)windowRef;

	if (acceptTextInput)
	{
		if (!SDL_StartTextInput(windowController->window))
		{
			fprintf(stderr, "Failed to start text input: %s\n", SDL_GetError());
		}
	}
	else
	{
		if (!SDL_StopTextInput(windowController->window))
		{
			fprintf(stderr, "Failed to stop text input: %s\n", SDL_GetError());
		}
	}
}

void ZGPollWindowAndInputEvents(ZGWindow *windowRef, const void *systemEvent)
{
	WindowController *windowController = (WindowController *)windowRef;
	if (systemEvent == NULL) return;
	
	const SDL_Event *sdlEvent = systemEvent;

	if (windowController->windowEventHandler == NULL && sdlEvent->type >= SDL_EVENT_WINDOW_FIRST && sdlEvent->type <= SDL_EVENT_WINDOW_LAST)
	{
		return;
	}

	switch (sdlEvent->type)
	{
		case SDL_EVENT_WINDOW_FOCUS_LOST:
		{
			ZGWindowEvent event;
			event.type = ZGWindowEventTypeFocusLost;

			windowController->windowEventHandler(event, windowController->windowEventHandlerContext);
		} break;
		case SDL_EVENT_WINDOW_FOCUS_GAINED:
		{
			ZGWindowEvent event;
			event.type = ZGWindowEventTypeFocusGained;

			windowController->windowEventHandler(event, windowController->windowEventHandlerContext);
		} break;
		case SDL_EVENT_WINDOW_HIDDEN:
		{
			ZGWindowEvent event;
			event.type = ZGWindowEventTypeHidden;

			windowController->windowEventHandler(event, windowController->windowEventHandlerContext);
		} break;
		case SDL_EVENT_WINDOW_SHOWN:
		{
			ZGWindowEvent event;
			event.type = ZGWindowEventTypeShown;
			
			windowController->windowEventHandler(event, windowController->windowEventHandlerContext);
		} break;
		case SDL_EVENT_WINDOW_RESIZED:
		{
			ZGWindowEvent event;
			event.type = ZGWindowEventTypeResize;
			event.width = sdlEvent->window.data1;
			event.height = sdlEvent->window.data2;
			
			windowController->windowEventHandler(event, windowController->windowEventHandlerContext);
		} break;
		case SDL_EVENT_KEY_DOWN:
		{
			if (ZGTestReturnKeyCode(sdlEvent->key.scancode) && ((sdlEvent->key.mod & (SDL_KMOD_LALT | SDL_KMOD_RALT)) != 0))
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
				event.keyCode = sdlEvent->key.scancode;
				event.keyModifier = sdlEvent->key.mod;
				event.timestamp = sdlEvent->key.timestamp;
				
				windowController->keyboardEventHandler(event, windowController->keyboardEventHandlerContext);
			}
		} break;
		case SDL_EVENT_KEY_UP:
		{
			if (windowController->keyboardEventHandler != NULL)
			{
				ZGKeyboardEvent event;
				event.type = ZGKeyboardEventTypeKeyUp;
				event.keyCode = sdlEvent->key.scancode;
				event.keyModifier = sdlEvent->key.mod;
				event.timestamp = sdlEvent->key.timestamp;
				
				windowController->keyboardEventHandler(event, windowController->keyboardEventHandlerContext);
			}
		} break;
		case SDL_EVENT_TEXT_INPUT:
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
	return (SDL_GetWindowFlags(windowController->window) & SDL_WINDOW_FULLSCREEN) != 0;
}

bool _ZGSetWindowFullscreen(ZGWindow *windowRef, bool enabled, const char **errorString)
{
	WindowController *windowController = (WindowController *)windowRef;
	bool result = SDL_SetWindowFullscreen(windowController->window, enabled);
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

