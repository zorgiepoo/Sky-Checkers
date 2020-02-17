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
#include "gamepad.h"

#if PLATFORM_IOS
#include "touch.h"
#else
#include "keyboard.h"
#include "renderer.h"
#endif

void initMenus(ZGWindow *window, GameState *gameState, void (*exitGame)(ZGWindow *));

void showPauseMenu(ZGWindow *window, GameState *gameState);

#if PLATFORM_IOS
void showGameMenus(ZGWindow *window);
#else
void drawMenus(Renderer *renderer);

void performKeyboardMenuAction(ZGKeyboardEvent *event, GameState *gameState, ZGWindow *window);
void performKeyboardMenuTextInputAction(ZGKeyboardEvent *event);
#endif

#if PLATFORM_TVOS
void performMenuTapAction(ZGWindow *window, GameState *gameState);
#endif

void performGamepadMenuAction(GamepadEvent *event, GameState *gameState, ZGWindow *window);
