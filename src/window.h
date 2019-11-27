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
#include <stdio.h>
#include <stdbool.h>

typedef void ZGWindow;

typedef enum
{
	ZG_WINDOW_FLAG_NONE,
	ZG_WINDOW_FLAG_FULLSCREEN,
#ifdef linux
	ZG_WINDOW_FLAG_OPENGL,
#endif
} ZGWindowFlags;

ZGWindow *ZGCreateWindow(const char *windowTitle, int32_t windowWidth, int32_t windowHeight, ZGWindowFlags flags);
void ZGDestroyWindow(ZGWindow *window);

void ZGWindowHideCursor(ZGWindow *window);

bool ZGWindowHasFocus(ZGWindow *window);
bool ZGWindowIsFullscreen(ZGWindow *window);

bool ZGSetWindowFullscreen(ZGWindow *window, bool enabled, const char **errorString);
