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

#include "window.h"
#include "sdl.h"

typedef struct
{
	SDL_Window *window;
	void (*eventHandler)(ZGWindowEvent, void *);
	void *eventHandlerContext;
} WindowController;

ZGWindow *ZGCreateWindow(const char *windowTitle, int32_t windowWidth, int32_t windowHeight, bool *fullscreenFlag)
{
	Uint32 videoFlags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
#ifdef linux
	videoFlags |= SDL_WINDOW_OPENGL;
#endif
	if (fullscreenFlag != NULL && *fullscreenFlag)
	{
		videoFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}
	
	WindowController *windowController = calloc(1, sizeof(*windowController));
	windowController->window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, videoFlags);
	return windowController;
}

void ZGDestroyWindow(ZGWindow *windowRef)
{
	WindowController *windowController = (WindowController *)windowRef;
	SDL_DestroyWindow(windowController->window);
	
	windowController->window = NULL;
	windowController->eventHandler = NULL;
	windowController->eventHandlerContext = NULL;
	
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
	windowController->eventHandler = windowEventHandler;
	windowController->eventHandlerContext = context;
}

void ZGPollWindowEvents(ZGWindow *windowRef, const void *systemEvent)
{
	WindowController *windowController = (WindowController *)windowRef;
	if (windowController->eventHandler == NULL || systemEvent == NULL) return;
	
	const SDL_Event *sdlEvent = systemEvent;
	switch (sdlEvent->type)
	{
		case SDL_WINDOWEVENT:
		{
			switch (sdlEvent->window.event)
			{
				case SDL_WINDOWEVENT_FOCUS_LOST:
				{
					ZGWindowEvent event;
					event.type = ZGWindowEventTypeFocusLost;

					windowController->eventHandler(event, windowController->eventHandlerContext);
				} break;
				case SDL_WINDOWEVENT_FOCUS_GAINED:
				{
					ZGWindowEvent event;
					event.type = ZGWindowEventTypeFocusGained;

					windowController->eventHandler(event, windowController->eventHandlerContext);
				} break;
				case SDL_WINDOWEVENT_HIDDEN:
				{
					ZGWindowEvent event;
					event.type = ZGWindowEventTypeHidden;

					windowController->eventHandler(event, windowController->eventHandlerContext);
				} break;
				case SDL_WINDOWEVENT_SHOWN:
				{
					ZGWindowEvent event;
					event.type = ZGWindowEventTypeShown;
					
					windowController->eventHandler(event, windowController->eventHandlerContext);
				} break;
				case SDL_WINDOWEVENT_RESIZED:
				{
					ZGWindowEvent event;
					event.type = ZGWindowEventTypeResize;
					event.width = sdlEvent->window.data1;
					event.height = sdlEvent->window.data2;
					
					windowController->eventHandler(event, windowController->eventHandlerContext);
				} break;
			}
		}
	}
}

#ifdef linux
bool ZGWindowIsFullscreen(ZGWindow *windowRef)
{
	WindowController *windowController = (WindowController *)windowRef;
	return (SDL_GetWindowFlags(windowController->window) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) != 0;
}

bool ZGSetWindowFullscreen(ZGWindow *windowRef, bool enabled, const char **errorString)
{
	WindowController *windowController = (WindowController *)windowRef;
	bool result = SDL_SetWindowFullscreen(windowController->window, enabled ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) == 0;
	if (!result && errorString != NULL)
	{
		*errorString = SDL_GetError();
	}
	return result;
}
#endif

#ifdef WINDOWS
void ZGSetWindowMinimumSize(ZGWindow *windowRef, int32_t minWidth, int32_t minHeight)
{
	WindowController *windowController = (WindowController *)windowRef;
	SDL_SetWindowMinimumSize(windowController->window, minWidth, minHeight);
}

void ZGGetWindowSize(ZGWindow *windowRef, int32_t *width, int32_t *height)
{
	WindowController *windowController = (WindowController *)windowRef;
	SDL_GetWindowSize(windowController->windowRef, width, height);
}
#endif

#ifndef linux
void *ZGWindowHandle(ZGWindow *windowRef)
{
	WindowController *windowController = (WindowController *)windowRef;
	
	SDL_SysWMinfo systemInfo;
	SDL_VERSION(&systemInfo.version);
	SDL_GetWindowWMInfo(windowController->window, &systemInfo);
	
#ifdef MAC_OS_X
	return systemInfo.info.cocoa.window;
#endif
	
#ifdef WINDOWS
	return systemInfo.info.win.window;
#endif
}
#endif
