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
	ZGWindowEventTypeHidden,
#if PLATFORM_WINDOWS
	ZGWindowEventDeviceConnected
#endif
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
void ZGShowWindow(ZGWindow* window);

// I should make a version for Linux too
void ZGGetDrawableSize(ZGWindow* windowRef, int32_t* width, int32_t* height);
#endif

void ZGGetWindowSize(ZGWindow *window, int32_t *width, int32_t *height);

#if !PLATFORM_IOS
void ZGSetWindowEventHandler(ZGWindow *window, void *context, void (*windowEventHandler)(ZGWindowEvent, void *));
#endif

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

#if PLATFORM_LINUX
void ZGPollWindowAndInputEvents(ZGWindow *window, const void *systemEvent);
#endif

void *ZGWindowHandle(ZGWindow *window);
