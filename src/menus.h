/*
 * Copyright 2010 Mayur Pawashe
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
#include "globals.h"
#include "window.h"
#include "renderer.h"

#ifdef IOS_DEVICE
#include "touch.h"
#else
#include "keyboard.h"
#endif

void initMenus(void);

void drawMenus(Renderer *renderer);

#ifdef IOS_DEVICE
void performTouchMenuAction(ZGTouchEvent *event, ZGWindow *window);
#else
void performKeyboardMenuAction(ZGKeyboardEvent *event, GameState *gameState, ZGWindow *window);
void performKeyboardMenuTextInputAction(ZGKeyboardEvent *event);
#endif
