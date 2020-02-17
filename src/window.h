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

#if PLATFORM_IOS
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

#if PLATFORM_WINDOWS
void ZGSetWindowMinimumSize(ZGWindow *window, int32_t minWidth, int32_t minHeight);
#endif

void ZGGetWindowSize(ZGWindow *window, int32_t *width, int32_t *height);

void ZGSetWindowEventHandler(ZGWindow *window, void *context, void (*windowEventHandler)(ZGWindowEvent, void *));

#if PLATFORM_LINUX
bool ZGWindowIsFullscreen(ZGWindow *window);
#endif

#if PLATFORM_IOS
void ZGSetTouchEventHandler(ZGWindow *window, void *context, void (*touchEventHandler)(ZGTouchEvent, void *));

#if PLATFORM_TVOS
void ZGInstallMenuGesture(ZGWindow *window);
void ZGUninstallMenuGesture(ZGWindow *window);
#else
void ZGInstallTouchGestures(ZGWindow *window);
void ZGUninstallTouchGestures(ZGWindow *window);
#endif

#else
void ZGSetKeyboardEventHandler(ZGWindow *window, void *context, void (*keyboardEventHandler)(ZGKeyboardEvent, void *));
#endif

void ZGPollWindowAndInputEvents(ZGWindow *window, const void *systemEvent);

void *ZGWindowHandle(ZGWindow *window);
