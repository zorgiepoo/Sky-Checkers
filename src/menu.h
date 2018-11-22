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

#include "maincore.h"
#include "math_3d.h"
#include "renderer.h"

typedef struct _Menu
{
	struct _Menu *above;
	struct _Menu *under;
	
	// The child on the top of the child list.
	struct _Menu *mainChild;
	
	// This is the last child that you've went to that was a child of the parent.
	// if the favorite child exists, then we go to it instead of the main child.
	struct _Menu *favoriteChild;
	
	struct _Menu *parent;
	
	// Function that gets called when the menu is invoked.
	// invokeMenu() calls this, of which you have to call yourself (like when a return key is pressed for example).
	void (*action)(void *context);
	
	// drawMenus() calls this function to draw the menu.
	// you have control over how this function draws.
	void (*draw)(Renderer *renderer, color4_t preferredColor);
} Menu;

extern Menu gMainMenu;
extern Menu *gCurrentMenu;

void initMainMenu(void);

void addSubMenu(Menu *parentMenu, Menu *childMenu);

SDL_bool isChildBeingDrawn(Menu *child);

// need to call these yourself.
void invokeMenu(void *context);
void drawMenus(Renderer *renderer);
void changeMenu(int direction);
