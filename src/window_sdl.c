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

ZGWindow *ZGCreateWindow(const char *windowTitle, int32_t windowWidth, int32_t windowHeight, ZGWindowFlags flags)
{
	Uint32 videoFlags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
#ifdef linux
	videoFlags |= SDL_WINDOW_OPENGL;
#endif
	if ((flags & ZG_WINDOW_FLAG_FULLSCREEN) != 0)
	{
		videoFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}
	
	return SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, videoFlags);
}

void ZGDestroyWindow(ZGWindow *window)
{
	SDL_DestroyWindow(window);
}

void ZGWindowHideCursor(ZGWindow *window)
{
	SDL_ShowCursor(SDL_DISABLE);
}

bool ZGWindowHasFocus(ZGWindow *window)
{
	return (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;
}

#ifdef linux
bool ZGWindowIsFullscreen(ZGWindow *window)
{
	return (SDL_GetWindowFlags(window) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) != 0;
}

bool ZGSetWindowFullscreen(ZGWindow *window, bool enabled, const char **errorString)
{
	bool result = SDL_SetWindowFullscreen(window, enabled ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) == 0;
	if (!result && errorString != NULL)
	{
		*errorString = SDL_GetError();
	}
	return result;
}
#endif

#ifdef WINDOWS
void ZGSetWindowMinimumSize(ZGWindow *window, int32_t minWidth, int32_t minHeight)
{
	SDL_SetWindowMinimumSize(window, minWidth, minHeight);
}

void ZGGetWindowSize(ZGWindow *window, int32_t *width, int32_t *height)
{
	SDL_GetWindowSize(window, width, height);
}
#endif

#ifndef linux
void *ZGWindowHandle(ZGWindow *window)
{
	SDL_SysWMinfo systemInfo;
	SDL_VERSION(&systemInfo.version);
	SDL_GetWindowWMInfo(window, &systemInfo);
	
#ifdef MAC_OS_X
	return systemInfo.info.cocoa.window;
#endif
	
#ifdef WINDOWS
	return systemInfo.info.win.window;
#endif
}
#endif
