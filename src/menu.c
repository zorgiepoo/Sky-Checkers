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

/* Menu callback system! */

#include "menu.h"
#include "characters.h" // for constants RIGHT, LEFT, UP, DOWN
#include "utilities.h"
#include "audio.h"

/*
 * Everyone is a type of child of gMainMenu.
 * gMainMenu has no parent.
 */
Menu gMainMenu;
// the current selected menu.
Menu *gCurrentMenu = NULL;

void initMainMenu(void)
{
	gMainMenu.above = NULL;
	gMainMenu.under = NULL;
	gMainMenu.parent = NULL;
	gMainMenu.mainChild = NULL;
	gMainMenu.favoriteChild = NULL;
	
	gMainMenu.draw = NULL;
	gMainMenu.action = NULL;
}

void addSubMenu(Menu *parentMenu, Menu *childMenu)
{
	// Prepare the child.
	childMenu->above = NULL;
	childMenu->under = NULL;
	
	// These two are important to do if the child becomes a parent later on.
	childMenu->mainChild = NULL;
	childMenu->favoriteChild = NULL;
	
	if (parentMenu->mainChild == NULL)
	{
		// parent has no childs and is new.
		// add the mainChild.
		parentMenu->mainChild = childMenu;
		
		// set gCurrentMenu to the child if it's the first child of gMainMenu.
		if (parentMenu == &gMainMenu)
			gCurrentMenu = childMenu;
	}
	else
	{
		if (parentMenu->mainChild->under == NULL)
		{
			// There's only one child (the mainChild) under this parent.
			
			// Add the second child and make the connections.
			parentMenu->mainChild->under = childMenu;
			childMenu->under = parentMenu->mainChild;
			childMenu->above = parentMenu->mainChild;
			parentMenu->mainChild->above = childMenu;
		}
		else
		{
			Menu *secondLastMenu;
			
			// This is the last menu for now, but it'll become the second last one.
			secondLastMenu = parentMenu->mainChild->above;
			
			// Add the child.
			secondLastMenu->under = childMenu;
			
			// Make connections.
			childMenu->under = parentMenu->mainChild;
			parentMenu->mainChild->above = childMenu;
			childMenu->above = secondLastMenu;
		}
	}
	
	childMenu->parent = parentMenu;
}

SDL_bool isChildBeingDrawn(Menu *child)
{
	return child->parent == gCurrentMenu->parent;
}

void invokeMenu(void)
{	
	if (gAudioEffectsFlag)
	{
		playMenuSound();
	}
	
	if (gCurrentMenu->action != NULL)
		gCurrentMenu->action();
	else
		zgPrint("gCurrentMenu's action() function is NULL");
}

void drawMenus(Renderer *renderer)
{
	Menu *theMenu;
	
	if (gCurrentMenu->parent == NULL)
	{
		zgPrint("Parent is NULL drawMenu():");
		return;
	}
	
	// Don't enter in the while loop if there's only one item.
	if (gCurrentMenu->under == NULL)
	{
		if (gCurrentMenu->draw != NULL)
		{
			// Use a selected color
			gCurrentMenu->draw(renderer, (color4_t){1.0f, 1.0f, 1.0f, 0.7f});
		}
		else
		{
			zgPrint("Current menu's draw function is NULL. Under if (gCurrentMenu->under == NULL) condition");
		}
		return;
	}
	
	theMenu = gCurrentMenu;
	
	// iterate through menus and draw each one of them.
	while ((theMenu = theMenu->under) != gCurrentMenu)
	{
		if (theMenu->draw != NULL)
		{
			// Use a non-selected color
			theMenu->draw(renderer, (color4_t){179.0f / 255.0f, 179.0f / 255.0f, 179.0f / 255.0f, 0.5f});
		}
		else
		{
			zgPrint("theMenu's draw function is NULL. Under while ((theMenu = theMenu->under) != gCurrentMenu)");
		}
	}
	
	// draw the last child that didn't get to be in the loop
	if (gCurrentMenu->draw != NULL)
	{
		gCurrentMenu->draw(renderer, (color4_t){1.0f, 1.0f, 1.0f, 0.7f});
	}
	else
	{
		zgPrint("Current menu's draw function is NULL. Last step in drawMenus");
	}
}

void changeMenu(int direction)
{	
	if (direction == RIGHT)
	{
		if (gCurrentMenu->mainChild != NULL)
		{
			if (gCurrentMenu->favoriteChild != NULL)
				gCurrentMenu = gCurrentMenu->favoriteChild;
			else
				gCurrentMenu = gCurrentMenu->mainChild;
		}
	}
	else if (direction == LEFT)
	{
		if (gCurrentMenu->parent != NULL && gCurrentMenu->parent != &gMainMenu)
		{
			gCurrentMenu->parent->favoriteChild = gCurrentMenu;
			gCurrentMenu = gCurrentMenu->parent;
		}
	}
	else if (direction == DOWN)
	{
		if (gCurrentMenu->under != NULL)
			gCurrentMenu = gCurrentMenu->under;
	}
	else if (direction == UP)
	{
		if (gCurrentMenu->above != NULL)
			gCurrentMenu = gCurrentMenu->above;
	}
	
	if (gAudioEffectsFlag)
	{
		playMenuSound();
	}
}
