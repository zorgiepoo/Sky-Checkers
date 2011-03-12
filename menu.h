/*
 * Coder: Mayur Pawashe (Zorg)
 * http://zorg.tejat.net
 */

#pragma once

#include "maincore.h"

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
	void (*action)(void);
	
	// drawMenus() calls this function to draw the menu.
	// you have control over how this function draws.
	void (*draw)(void);
} Menu;

extern Menu gMainMenu;
extern Menu *gCurrentMenu;

void initMainMenu(void);

void addSubMenu(Menu *parentMenu, Menu *childMenu);

SDL_bool isChildBeingDrawn(Menu *child);

// need to call these yourself.
void invokeMenu(void);
void drawMenus(void);
void changeMenu(int direction);
