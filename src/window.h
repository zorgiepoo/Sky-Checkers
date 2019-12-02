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

#include "platforms.h"

#ifdef IOS_DEVICE
#include "touch.h"
#else
#include "keyboard.h"
#endif

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

typedef void ZGWindow;
typedef enum
{
	ZGWindowEventTypeResize,
	ZGWindowEventTypeFocusGained,
	ZGWindowEventTypeFocusLost,
	ZGWindowEventTypeShown,
	ZGWindowEventTypeHidden
} ZGWindowEventType;

typedef struct
{
	int32_t width;
	int32_t height;
	
	ZGWindowEventType type;
} ZGWindowEvent;

ZGWindow *ZGCreateWindow(const char *windowTitle, int32_t windowWidth, int32_t windowHeight, bool *fullscreenFlag);
void ZGDestroyWindow(ZGWindow *window);

bool ZGWindowHasFocus(ZGWindow *window);

#ifdef linux
bool ZGWindowIsFullscreen(ZGWindow *window);
bool ZGSetWindowFullscreen(ZGWindow *window, bool enabled, const char **errorString);
#endif

#ifdef WINDOWS
void ZGSetWindowMinimumSize(ZGWindow *window, int32_t minWidth, int32_t minHeight);
void ZGGetWindowSize(ZGWindow *window, int32_t *width, int32_t *height);
#endif

void ZGSetWindowEventHandler(ZGWindow *window, void *context, void (*windowEventHandler)(ZGWindowEvent, void *));

#ifdef IOS_DEVICE
void ZGSetTouchEventHandler(ZGWindow *window, void *context, void (*touchEventHandler)(ZGTouchEvent, void *));
#else
void ZGSetKeyboardEventHandler(ZGWindow *window, void *context, void (*keyboardEventHandler)(ZGKeyboardEvent, void *));
#endif

void ZGPollWindowAndInputEvents(ZGWindow *window, const void *systemEvent);

#ifndef linux
void *ZGWindowHandle(ZGWindow *window);
#endif
