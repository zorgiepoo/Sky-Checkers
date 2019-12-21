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

#include "menus.h"
#include "input.h"
#include "text.h"
#include "network.h"
#include "audio.h"
#include "math_3d.h"
#include "renderer.h"
#include "quit.h"
#include "platforms.h"
#include "menu_actions.h"

#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

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

static void initMainMenu(void);

static void addSubMenu(Menu *parentMenu, Menu *childMenu);

static void invokeMenu(void *context);
static void changeMenu(int direction);

static void (*gExitGame)(ZGWindow *);

static Menu *gPlayMenu;
static Menu *gNetworkServerPlayMenu;
static Menu *gConnectToNetworkGameMenu;
static Menu *gPauseResumeMenu;
static Menu *gPauseExitMenu;

static Menu *gConfigureLivesMenu;

static Menu *gAIModeOptionsMenu;

static Menu *gRedRoverPlayerOptionsMenu;
static Menu *gGreenTreePlayerOptionsMenu;
static Menu *gBlueLightningPlayerOptionsMenu;

static bool gDrawArrowsForCharacterLivesFlag;
static bool gDrawArrowsForAIModeFlag;
static bool gDrawArrowsForNumberOfNetHumansFlag;

static bool gNetworkAddressFieldIsActive;
static bool gNetworkUserNameFieldIsActive;

static bool gMenuPendingOnKeyCode = false;
static uint32_t *gMenuPendingKeyCode;

static char *convertKeyCodeToString(uint32_t theKeyCode);
static void configureKey(uint32_t *id);

static void writeNetworkAddressText(uint8_t text);
static void writeNetworkUserNameText(uint8_t text);
static void performNetworkAddressBackspace(void);
static void performNetworkUserNameBackspace(void);

void setPendingKeyCode(uint32_t keyCode);

// Everyone is a type of child of gMainMenu. gMainMenu has no parent.
static Menu gMainMenu;
// the current selected menu.
static Menu *gCurrentMenu = NULL;

static void initMainMenu(void)
{
	gMainMenu.above = NULL;
	gMainMenu.under = NULL;
	gMainMenu.parent = NULL;
	gMainMenu.mainChild = NULL;
	gMainMenu.favoriteChild = NULL;
	
	gMainMenu.draw = NULL;
	gMainMenu.action = NULL;
}

static void addSubMenu(Menu *parentMenu, Menu *childMenu)
{
	// If the child menu was attached to a previous parent, remove this attachment
	if (childMenu->parent != NULL)
	{
		if (childMenu->parent->mainChild == childMenu)
		{
			childMenu->parent->mainChild = NULL;
		}
		
		if (childMenu->parent->favoriteChild == childMenu)
		{
			childMenu->parent->favoriteChild = NULL;
		}
	}
	
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

static void invokeMenu(void *context)
{
	if (gAudioEffectsFlag)
	{
		playMenuSound();
	}
	
	if (gCurrentMenu->action != NULL)
		gCurrentMenu->action(context);
	else
		fprintf(stderr, "gCurrentMenu's action() function is NULL\n");
}

void drawMenus(Renderer *renderer)
{
	Menu *theMenu;
	
	if (gCurrentMenu->parent == NULL)
	{
		fprintf(stderr, "Parent is NULL drawMenu():\n");
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
			fprintf(stderr, "Current menu's draw function is NULL. Under if (gCurrentMenu->under == NULL) condition\n");
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
			fprintf(stderr, "theMenu's draw function is NULL. Under while ((theMenu = theMenu->under) != gCurrentMenu)\n");
		}
	}
	
	// draw the last child that didn't get to be in the loop
	if (gCurrentMenu->draw != NULL)
	{
		gCurrentMenu->draw(renderer, (color4_t){1.0f, 1.0f, 1.0f, 0.7f});
	}
	else
	{
		fprintf(stderr, "Current menu's draw function is NULL. Last step in drawMenus\n");
	}
}

static void changeMenu(int direction)
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

static void drawUpAndDownArrowTriangles(Renderer *renderer, mat4_t modelViewMatrix)
{
	static BufferArrayObject vertexArrayObject;
	static bool initializedBuffer;
	if (!initializedBuffer)
	{
		ZGFloat vertices[] =
		{
			0.0f, 0.6f, 0.0f, 1.0f,
			-0.2f, 0.2f, 0.0f, 1.0f,
			0.2f, 0.2f, 0.0f, 1.0f,
			
			0.0f, -0.6f, 0.0f, 1.0f,
			-0.2f, -0.2f, 0.0f, 1.0f,
			0.2f, -0.2f, 0.0f, 1.0f
		};
		
		vertexArrayObject = createVertexArrayObject(renderer, vertices, sizeof(vertices));
		initializedBuffer = true;
	}
	
	// Because all opaque objects should be rendered first, we will draw a transparent object
	// that matches all of the menu's
	drawVertices(renderer, modelViewMatrix, RENDERER_TRIANGLE_MODE, vertexArrayObject, 6, (color4_t){0.0f, 0.0f, 0.5f, 0.8f}, RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA);
}

/* A bunch of menu drawing and action functions! */

void drawPlayMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-1.0f / 14.0f, 15.0f / 14.0f, -20.0f});
	
	drawString(renderer, modelViewMatrix, preferredColor, 10.0f / 14.0f, 5.0f / 14.0f, "Play");
}

void playGameAction(void *context)
{
	addSubMenu(gPlayMenu, gPauseResumeMenu);
	addSubMenu(gPlayMenu, gPauseExitMenu);
	
	GameMenuContext *menuContext = context;
	initGame(menuContext->window, true);
	
}

void drawPauseResumeMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-1.0f / 14.0f, 15.0f / 14.0f, -20.0f});
	
	drawString(renderer, modelViewMatrix, preferredColor, 12.0f / 14.0f, 5.0f / 14.0f, "Resume");
}

void pauseResumeMenuAction(void *context)
{
	changeMenu(LEFT);
	
	GameMenuContext *menuContext = context;
	*menuContext->gameState = GAME_STATE_ON;
	unPauseMusic();
}

void drawPauseExitMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});
	drawString(renderer, modelViewMatrix, preferredColor, 8.0f / 14.0f, 5.0f / 14.0f, "Exit");
}

void pauseExitMenuAction(void *context)
{
	changeMenu(LEFT);
	GameMenuContext *menuContext = context;
	menuContext->exitGame(menuContext->window);
}

void drawNetworkPlayMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 14.0f / 14.0f, 5.0f / 14.0f, "Online");
}

void networkPlayMenuAction(void *context)
{
	changeMenu(RIGHT);
}

void drawNetworkUserNameFieldMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t nameLabelModelViewMatrix = m4_translation((vec3_t){-0.65f, -1.07f, -20.00f});
	drawString(renderer, nameLabelModelViewMatrix, preferredColor, 10.0f / 14.0f, 5.0f / 14.0f, "Name:");
	
	color4_t color  =  gNetworkUserNameFieldIsActive ? (color4_t){0.0f, 0.0f, 0.8f, 1.0f} : preferredColor;
	
	if (strlen(gUserNameString) > 0)
	{
		mat4_t nameModelViewMatrix = m4_mul(nameLabelModelViewMatrix, m4_translation((vec3_t){0.8f, 0.0f, 0.0f}));
		
		drawStringLeftAligned(renderer, nameModelViewMatrix, color, 0.0024f, gUserNameString);
	}
}

void networkUserNameFieldMenuAction(void *context)
{
	gNetworkUserNameFieldIsActive = !gNetworkUserNameFieldIsActive;
}

void writeNetworkUserNameText(uint8_t text)
{
	if (gUserNameStringIndex < MAX_USER_NAME_SIZE - 1)
	{
		gUserNameString[gUserNameStringIndex] = text;
		gUserNameString[gUserNameStringIndex + 1] = '\0';
		
		gUserNameStringIndex++;
	}
}

void performNetworkUserNameBackspace(void)
{	
	if (gUserNameStringIndex > 0)
	{
		gUserNameStringIndex--;
		gUserNameString[gUserNameStringIndex] = '\0';
	}
}

void drawNetworkServerMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 1.07f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 18.0f / 14.0f, 5.0f / 14.0f, "Make Game");
}

void networkServerMenuAction(void *context)
{
	changeMenu(RIGHT);
}

void drawNetworkServerPlayMenu(Renderer *renderer, color4_t preferredColor)
{
	{
		mat4_t translationMatrix = m4_translation((vec3_t){-1.0f / 14.0f, 50.0f / 14.0f, -280.0f / 14.0f});
		color4_t textColor = (color4_t){0.3f, 0.2f, 1.0f, 1.0f};
		drawString(renderer, translationMatrix, textColor, 30.0f / 14.0f, 5.0f / 14.0f, "(UDP port used is "NETWORK_PORT")");
	}
	
	mat4_t modelViewMatrix = m4_translation((vec3_t){-1.43f, 1.07f, -20.00f});
	drawString(renderer, modelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Start Game");
}

void networkServerPlayMenuAction(void *context)
{
	addSubMenu(gNetworkServerPlayMenu, gPauseResumeMenu);
	addSubMenu(gNetworkServerPlayMenu, gPauseExitMenu);
	
	GameMenuContext *menuContext = context;
	startNetworkGame(menuContext->window);
}

void drawNetworkServerNumberOfPlayersMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t numberPlayersModelViewMatrix = m4_translation((vec3_t){-1.43f, 0.0f, -20.00f});
	drawStringf(renderer, numberPlayersModelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Friends: %d", gNumberOfNetHumans);

	if (gDrawArrowsForNumberOfNetHumansFlag)
	{
		drawUpAndDownArrowTriangles(renderer, m4_translation((vec3_t){0.35f / 1.25f, 0.1f / 1.25f, -25.0f / 1.25f}));
	}
}

void networkServerNumberOfPlayersMenuAction(void *context)
{
	gDrawArrowsForNumberOfNetHumansFlag = !gDrawArrowsForNumberOfNetHumansFlag;
}

void drawNetworkClientMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 18.0f / 14.0f, 5.0f / 14.0f, "Join Game");
}

void networkClientMenuAction(void *context)
{
	changeMenu(RIGHT);
}

void drawNetworkAddressFieldMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t hostLabelModelViewMatrix = m4_translation((vec3_t){-3.86f, 0.00f, -20.00f});
	drawString(renderer, hostLabelModelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Host Address:");
	
	color4_t color = gNetworkAddressFieldIsActive ? (color4_t){0.0f, 0.0f, 0.8f, 1.0f} : preferredColor;
	
	if (strlen(gServerAddressString) > 0)
	{
		mat4_t hostModelViewMatrix = m4_mul(hostLabelModelViewMatrix, m4_translation((vec3_t){1.6f, 0.0f, 0.0f}));
		
		drawStringLeftAligned(renderer, hostModelViewMatrix, color, 0.0024f, gServerAddressString);
	}
	else if (gNetworkAddressFieldIsActive)
	{
		mat4_t translationMatrix = m4_translation((vec3_t){-1.0f / 14.0f, 50.0f / 14.0f, -280.0f / 14.0f});
		
		color4_t textColor = (color4_t){0.3f, 0.2f, 1.0f, 1.0f};
		
		drawString(renderer, translationMatrix, textColor, 60.0f / 14.0f, 5.0f / 14.0f, "(Feel free to paste in the host address!)");
	}
}

void networkAddressFieldMenuAction(void *context)
{
	gNetworkAddressFieldIsActive = !gNetworkAddressFieldIsActive;
}

void writeNetworkAddressText(uint8_t text)
{
	if (gServerAddressStringIndex < MAX_SERVER_ADDRESS_SIZE - 1)
	{
		gServerAddressString[gServerAddressStringIndex] = text;
		gServerAddressString[gServerAddressStringIndex + 1] = '\0';
		
		gServerAddressStringIndex++;
	}
}

void performNetworkAddressBackspace(void)
{	
	if (gServerAddressStringIndex > 0)
	{
		gServerAddressStringIndex--;
		gServerAddressString[gServerAddressStringIndex] = '\0';
	}
}

void drawConnectToNetworkGameMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-4.21f, -1.07f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Connect");
}

void connectToNetworkGameMenuAction(void *context)
{
	addSubMenu(gConnectToNetworkGameMenu, gPauseResumeMenu);
	addSubMenu(gConnectToNetworkGameMenu, gPauseExitMenu);
	
	GameMenuContext *menuContext = context;
	connectToNetworkGame(menuContext->gameState);
}

void drawGameOptionsMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -1.07f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 14.0f / 14.0f, 5.0f / 14.0f, "Options");
}

void gameOptionsMenuAction(void *context)
{
	changeMenu(RIGHT);
}

void drawPlayerOptionsMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 1.07f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Players");
}

void playerOptionsMenuAction(void *context)
{
	changeMenu(RIGHT);
}

void drawConfigureKeysMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Keyboard");
}

void configureKeysMenuAction(void *context)
{
	changeMenu(RIGHT);
}

void drawPinkBubbleGumPlayerOptionsMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 1.07f, -20.00f});	
	if (gPinkBubbleGum.state == CHARACTER_HUMAN_STATE)
		drawString(renderer, modelViewMatrix, preferredColor, 24.0f / 14.0f, 5.0f / 14.0f, "Pink Bubblegum: Human");
	else if (gPinkBubbleGum.state == CHARACTER_AI_STATE)
		drawString(renderer, modelViewMatrix, preferredColor, 24.0f / 14.0f, 5.0f / 14.0f, "Pink Bubblegum: Bot");
}

void pinkBubbleGumPlayerOptionsMenuAction(void *context)
{
	if (gPinkBubbleGum.state == CHARACTER_HUMAN_STATE)
	{
		gPinkBubbleGum.state = CHARACTER_AI_STATE;
		gRedRover.state = CHARACTER_AI_STATE;
		gGreenTree.state = CHARACTER_AI_STATE;
		gBlueLightning.state = CHARACTER_AI_STATE;
	}
	else
	{
		gPinkBubbleGum.state = CHARACTER_HUMAN_STATE;
	}
}

void drawRedRoverPlayerOptionsMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});
	color4_t color = (gPinkBubbleGum.state == CHARACTER_AI_STATE) ? (color4_t){0.0f, 0.0f, 0.0f, 0.6f} : preferredColor;
	
	if (gRedRover.state == CHARACTER_HUMAN_STATE)
	{
		drawString(renderer, modelViewMatrix, color, 24.0f / 14.0f, 5.0f / 14.0f, "Red Rover: Human");
	}
	else
	{
		drawString(renderer, modelViewMatrix, color, 24.0f / 14.0f, 5.0f / 14.0f, "Red Rover: Bot");
	}
}

void redRoverPlayerOptionsMenuAction(void *context)
{
	if (gRedRover.state == CHARACTER_HUMAN_STATE)
	{
		gRedRover.state = CHARACTER_AI_STATE;
		gGreenTree.state = CHARACTER_AI_STATE;
		gBlueLightning.state = CHARACTER_AI_STATE;
	}
	else
	{
		gRedRover.state = CHARACTER_HUMAN_STATE;
	}
}

void drawGreenTreePlayerOptionsMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -1.07f, -20.00f});
	color4_t color = (gRedRover.state == CHARACTER_AI_STATE) ? (color4_t){0.0f, 0.0f, 0.0f, 0.6f} : preferredColor;
	
	if (gGreenTree.state == CHARACTER_HUMAN_STATE)
	{
		drawString(renderer, modelViewMatrix, color, 24.0f / 14.0f, 5.0f / 14.0f, "Green Tree: Human");
	}
	else if (gGreenTree.state == CHARACTER_AI_STATE)
	{
		drawString(renderer, modelViewMatrix, color, 24.0f / 14.0f, 5.0f / 14.0f, "Green Tree: Bot");
	}
}

void greenTreePlayerOptionsMenuAction(void *context)
{
	if (gGreenTree.state == CHARACTER_HUMAN_STATE)
	{
		gGreenTree.state = CHARACTER_AI_STATE;
		gBlueLightning.state = CHARACTER_AI_STATE;
	}
	else
	{
		gGreenTree.state = CHARACTER_HUMAN_STATE;
	}
}

void drawBlueLightningPlayerOptionsMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -2.14f, -20.00f});
	color4_t color = (gGreenTree.state == CHARACTER_AI_STATE) ? (color4_t){0.0f, 0.0f, 0.0f, 0.6f} : preferredColor;
	
	if (gBlueLightning.state == CHARACTER_HUMAN_STATE)
	{
		drawString(renderer, modelViewMatrix, color, 24.0f / 14.0f, 5.0f / 14.0f, "Blue Lightning: Human");
	}
	else if (gBlueLightning.state == CHARACTER_AI_STATE)
	{
		drawString(renderer, modelViewMatrix, color, 24.0f / 14.0f, 5.0f / 14.0f, "Blue Lightning: Bot");
	}
}

void blueLightningPlayerOptionsMenuAction(void *context)
{
	if (gBlueLightning.state == CHARACTER_HUMAN_STATE)
	{
		gBlueLightning.state = CHARACTER_AI_STATE;
	}
	else
	{
		gBlueLightning.state = CHARACTER_HUMAN_STATE;
	}
}

void drawAIModeOptionsMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modeModelViewMatrix = m4_translation((vec3_t){-0.07f, -3.21f, -20.00f});	
	if (gAIMode == AI_EASY_MODE)
		drawString(renderer, modeModelViewMatrix, preferredColor, 24.0f / 14.0f, 5.0f / 14.0f, "Bot Mode: Easy");
	
	else if (gAIMode == AI_MEDIUM_MODE)
		drawString(renderer, modeModelViewMatrix, preferredColor, 24.0f / 14.0f, 5.0f / 14.0f, "Bot Mode: Medium");
	
	else if (gAIMode == AI_HARD_MODE)
		drawString(renderer, modeModelViewMatrix, preferredColor, 24.0f / 14.0f, 5.0f / 14.0f, "Bot Mode: Hard");
	
	if (gDrawArrowsForAIModeFlag)
	{
		drawUpAndDownArrowTriangles(renderer, m4_translation((vec3_t){2.4f / 1.25f, -4.1f / 1.25f, -25.0f / 1.25f}));
	}
}

void drawConfigureLivesMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t livesModelViewMatrix = m4_translation((vec3_t){-0.07f, -4.29f, -20.00f});	
	drawStringf(renderer, livesModelViewMatrix, preferredColor, 24.0f / 14.0f, 5.0f / 14.0f, "Player Lives: %i", gCharacterLives);
	
	if (gDrawArrowsForCharacterLivesFlag)
	{
		drawUpAndDownArrowTriangles(renderer, m4_translation((vec3_t){2.4f / 1.25f, -5.4f / 1.25f, -25.0f / 1.25f}));
	}
}

void drawPinkBubbleGumConfigKey(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 1.07f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 18.0f / 14.0f, 5.0f / 14.0f, "Pink Bubblegum");
}

void pinkBubbleGumKeyMenuAction(void *context)
{
	changeMenu(RIGHT);
}

void drawRedRoverConfigKey(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 18.0f / 14.0f, 5.0f / 14.0f, "Red Rover");
}

void redRoverKeyMenuAction(void *context)
{
	changeMenu(RIGHT);
}

void drawGreenTreeConfigKey(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -1.07f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 18.0f / 14.0f, 5.0f / 14.0f, "Green Tree");
}

void greenTreeKeyMenuAction(void *context)
{
	changeMenu(RIGHT);
}

void drawBlueLightningConfigKey(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -2.14f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 18.0f / 14.0f, 5.0f / 14.0f, "Blue Lightning");
}

void blueLightningKeyMenuAction(void *context)
{
	changeMenu(RIGHT);
}

// start configuration menus

static void drawKeyboardConfigurationInstructions(Renderer *renderer)
{
	mat4_t translationMatrix = m4_translation((vec3_t){-1.0f / 14.0f, 50.0f / 14.0f, -280.0f / 14.0f});
	
	color4_t textColor = (color4_t){0.3f, 0.2f, 1.0f, 1.0f};
	
	drawString(renderer, translationMatrix, textColor, 100.0f / 14.0f, 5.0f / 14.0f, "Enter: map keyboard input; Escape: cancel");
}

void drawPinkBubbleGumConfigRightKey(Renderer *renderer, color4_t preferredColor)
{
	drawKeyboardConfigurationInstructions(renderer);
	
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 1.07f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Right: %s", convertKeyCodeToString(gPinkBubbleGumInput.r_id));
}

void pinkBubbleGumRightKeyMenuAction(void *context)
{	
	configureKey(&gPinkBubbleGumInput.r_id);
}

void drawPinkBubbleGumConfigLeftKey(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Left: %s", convertKeyCodeToString(gPinkBubbleGumInput.l_id));
}

void pinkBubbleGumLeftKeyMenuAction(void *context)
{
	configureKey(&gPinkBubbleGumInput.l_id);
}

void drawPinkBubbleGumConfigUpKey(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -1.07f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Up: %s", convertKeyCodeToString(gPinkBubbleGumInput.u_id));
}

void pinkBubbleGumUpKeyMenuAction(void *context)
{
	configureKey(&gPinkBubbleGumInput.u_id);
}

void drawPinkBubbleGumConfigDownKey(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -2.14f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Down: %s", convertKeyCodeToString(gPinkBubbleGumInput.d_id));
}

void pinkBubbleGumDownKeyMenuAction(void *context)
{
	configureKey(&gPinkBubbleGumInput.d_id);
}

void drawPinkBubbleGumConfigFireKey(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -3.21f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Fire: %s", convertKeyCodeToString(gPinkBubbleGumInput.weap_id));
}

void pinkBubbleGumFireKeyMenuAction(void *context)
{
	configureKey(&gPinkBubbleGumInput.weap_id);
}

void drawRedRoverConfigRightKey(Renderer *renderer, color4_t preferredColor)
{
	drawKeyboardConfigurationInstructions(renderer);
	
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 1.07f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Right: %s", convertKeyCodeToString(gRedRoverInput.r_id));
}

void redRoverRightKeyMenuAction(void *context)
{
	configureKey(&gRedRoverInput.r_id);
}

void drawRedRoverConfigLeftKey(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Left: %s", convertKeyCodeToString(gRedRoverInput.l_id));
}

void redRoverLeftKeyMenuAction(void *context)
{
	configureKey(&gRedRoverInput.l_id);
}

void drawRedRoverConfigUpKey(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -1.07f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Up: %s", convertKeyCodeToString(gRedRoverInput.u_id));
}

void redRoverUpKeyMenuAction(void *context)
{
	configureKey(&gRedRoverInput.u_id);
}

void drawRedRoverConfigDownKey(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -2.14f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Down: %s", convertKeyCodeToString(gRedRoverInput.d_id));
}

void redRoverDownKeyMenuAction(void *context)
{
	configureKey(&gRedRoverInput.d_id);
}

void drawRedRoverConfigFireKey(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -3.21f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Fire: %s", convertKeyCodeToString(gRedRoverInput.weap_id));
}

void redRoverFireKeyMenuAction(void *context)
{
	configureKey(&gRedRoverInput.weap_id);
}

void drawGreenTreeConfigRightKey(Renderer *renderer, color4_t preferredColor)
{
	drawKeyboardConfigurationInstructions(renderer);
	
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 1.07f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Right: %s", convertKeyCodeToString(gGreenTreeInput.r_id));
}

void greenTreeRightKeyMenuAction(void *context)
{
	configureKey(&gGreenTreeInput.r_id);
}

void drawGreenTreeConfigLeftKey(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Left: %s", convertKeyCodeToString(gGreenTreeInput.l_id));
}

void greenTreeLeftKeyMenuAction(void *context)
{
	configureKey(&gGreenTreeInput.l_id);
}

void drawGreenTreeConfigUpKey(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -1.07f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Up: %s", convertKeyCodeToString(gGreenTreeInput.u_id));
}

void greenTreeUpKeyMenuAction(void *context)
{
	configureKey(&gGreenTreeInput.u_id);
}

void drawGreenTreeConfigDownKey(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -2.14f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Down: %s", convertKeyCodeToString(gGreenTreeInput.d_id));
}

void greenTreeDownKeyMenuAction(void *context)
{
	configureKey(&gGreenTreeInput.d_id);
}

void drawGreenTreeConfigFireKey(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -3.21f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Fire: %s", convertKeyCodeToString(gGreenTreeInput.weap_id));
}

void greenTreeFireKeyMenuAction(void *context)
{
	configureKey(&gGreenTreeInput.weap_id);
}

void drawBlueLightningConfigRightKey(Renderer *renderer, color4_t preferredColor)
{
	drawKeyboardConfigurationInstructions(renderer);
	
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 1.07f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Right: %s", convertKeyCodeToString(gBlueLightningInput.r_id));
}

void blueLightningRightKeyMenuAction(void *context)
{
	configureKey(&gBlueLightningInput.r_id);
}

void drawBlueLightningConfigLeftKey(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Left: %s", convertKeyCodeToString(gBlueLightningInput.l_id));
}

void blueLightningLeftKeyMenuAction(void *context)
{
	configureKey(&gBlueLightningInput.l_id);
}

void drawBlueLightningConfigUpKey(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -1.07f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Up: %s", convertKeyCodeToString(gBlueLightningInput.u_id));
}

void blueLightningUpKeyMenuAction(void *context)
{
	configureKey(&gBlueLightningInput.u_id);
}

void drawBlueLightningConfigDownKey(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -2.14f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Down: %s", convertKeyCodeToString(gBlueLightningInput.d_id));
}

void blueLightningDownKeyMenuAction(void *context)
{
	configureKey(&gBlueLightningInput.d_id);
}

void drawBlueLightningConfigFireKey(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -3.21f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Fire: %s", convertKeyCodeToString(gBlueLightningInput.weap_id));
}

void blueLightningFireKeyMenuAction(void *context)
{
	configureKey(&gBlueLightningInput.weap_id);
}

// Audio options
void drawAudioOptionsMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -1.07f, -20.00f});
	drawString(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Sounds");
}

void audioOptionsMenuAction(void *context)
{
	changeMenu(RIGHT);
}

void drawAudioEffectsOptionsMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 1.07f, -20.00f});	
	if (gAudioEffectsFlag)
	{
		drawString(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Effects: On");
	}
	else
	{
		drawString(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Effects: Off");
	}
}

void audioEffectsOptionsMenuAction(void *context)
{
	gAudioEffectsFlag = !gAudioEffectsFlag;
}

void drawAudioMusicOptionsMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});	
	if (gAudioMusicFlag)
	{
		drawString(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Music: On");
	}
	else
	{
		drawString(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Music: Off");
	}
}

void audioMusicOptionsMenuAction(void *context)
{
	GameMenuContext *menuContext = context;
	
	updateAudioMusic(menuContext->window, !gAudioMusicFlag);
}

void drawQuitMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -2.14f, -20.00f});
	drawString(renderer, modelViewMatrix, preferredColor, 10.0f / 14.0f, 5.0f / 14.0f, "Quit");
}

void quitMenuAction(void *context)
{
	ZGSendQuitEvent();
}

void initMenus(ZGWindow *window, GameState *gameState, void (*exitGame)(ZGWindow *))
{
	initMainMenu();
	
	gPlayMenu =									calloc(1, sizeof(Menu));
	gPauseResumeMenu = 							calloc(1, sizeof(Menu));
	gPauseExitMenu = 							calloc(1, sizeof(Menu));
	Menu *networkPlayMenu =						calloc(1, sizeof(Menu));
	Menu *networkServerMenu =					calloc(1, sizeof(Menu));
	gNetworkServerPlayMenu =					calloc(1, sizeof(Menu));
	Menu *networkServerNumberOfPlayersMenu =	calloc(1, sizeof(Menu));
	Menu *networkClientMenu =					calloc(1, sizeof(Menu));
	Menu *networkUserNameMenu =					calloc(1, sizeof(Menu));
	Menu *networkAddressFieldMenu =				calloc(1, sizeof(Menu));
	gConnectToNetworkGameMenu =					calloc(1, sizeof(Menu));
	Menu *gameOptionsMenu =						calloc(1, sizeof(Menu));
	Menu *playerOptionsMenu =					calloc(1, sizeof(Menu));
	Menu *pinkBubbleGumPlayerOptionsMenu =		calloc(1, sizeof(Menu));
	gRedRoverPlayerOptionsMenu	=				calloc(1, sizeof(Menu));
	gGreenTreePlayerOptionsMenu =				calloc(1, sizeof(Menu));
	gBlueLightningPlayerOptionsMenu =			calloc(1, sizeof(Menu));
	gAIModeOptionsMenu =						calloc(1, sizeof(Menu));
	gConfigureLivesMenu =						calloc(1, sizeof(Menu));
	Menu *configureKeysMenu =					calloc(1, sizeof(Menu));
	// Four characters that each have their own menu + five configured menu actions (right, up, left, down, fire)
	Menu *characterConfigureKeys = 				calloc(4 * 6, sizeof(Menu));
	Menu *audioOptionsMenu =					calloc(1, sizeof(Menu));
	Menu *audioEffectsOptionsMenu =				calloc(1, sizeof(Menu));
	Menu *audioMusicOptionsMenu =				calloc(1, sizeof(Menu));
	Menu *quitMenu =							calloc(1, sizeof(Menu));
	
	gExitGame = exitGame;
	
	// set action and drawing functions
	gPlayMenu->draw = drawPlayMenu;
	gPlayMenu->action = playGameAction;
	
	gPauseResumeMenu->draw = drawPauseResumeMenu;
	gPauseResumeMenu->action = pauseResumeMenuAction;
	
	gPauseExitMenu->draw = drawPauseExitMenu;
	gPauseExitMenu->action = pauseExitMenuAction;
	
	networkPlayMenu->draw = drawNetworkPlayMenu;
	networkPlayMenu->action = networkPlayMenuAction;
	
	networkServerMenu->draw = drawNetworkServerMenu;
	networkServerMenu->action = networkServerMenuAction;
	
	gNetworkServerPlayMenu->draw = drawNetworkServerPlayMenu;
	gNetworkServerPlayMenu->action = networkServerPlayMenuAction;
	
	networkServerNumberOfPlayersMenu->draw = drawNetworkServerNumberOfPlayersMenu;
	networkServerNumberOfPlayersMenu->action = networkServerNumberOfPlayersMenuAction;
	
	networkClientMenu->draw = drawNetworkClientMenu;
	networkClientMenu->action = networkClientMenuAction;
	
	networkUserNameMenu->draw = drawNetworkUserNameFieldMenu;
	networkUserNameMenu->action = networkUserNameFieldMenuAction;
	
	networkAddressFieldMenu->draw = drawNetworkAddressFieldMenu;
	networkAddressFieldMenu->action = networkAddressFieldMenuAction;
	
	gConnectToNetworkGameMenu->draw = drawConnectToNetworkGameMenu;
	gConnectToNetworkGameMenu->action = connectToNetworkGameMenuAction;
	
	gameOptionsMenu->draw = drawGameOptionsMenu;
	gameOptionsMenu->action = gameOptionsMenuAction;
	
	playerOptionsMenu->draw = drawPlayerOptionsMenu;
	playerOptionsMenu->action = playerOptionsMenuAction;
	
	pinkBubbleGumPlayerOptionsMenu->draw = drawPinkBubbleGumPlayerOptionsMenu;
	pinkBubbleGumPlayerOptionsMenu->action = pinkBubbleGumPlayerOptionsMenuAction;
	
	gRedRoverPlayerOptionsMenu->draw = drawRedRoverPlayerOptionsMenu;
	gRedRoverPlayerOptionsMenu->action = redRoverPlayerOptionsMenuAction;
	
	gGreenTreePlayerOptionsMenu->draw = drawGreenTreePlayerOptionsMenu;
	gGreenTreePlayerOptionsMenu->action = greenTreePlayerOptionsMenuAction;
	
	gBlueLightningPlayerOptionsMenu->draw = drawBlueLightningPlayerOptionsMenu;
	gBlueLightningPlayerOptionsMenu->action = blueLightningPlayerOptionsMenuAction;
	
	gAIModeOptionsMenu->draw = drawAIModeOptionsMenu;
	gAIModeOptionsMenu->action = NULL;
	
	gConfigureLivesMenu->draw = drawConfigureLivesMenu;
	gConfigureLivesMenu->action = NULL;
	
	configureKeysMenu->draw = drawConfigureKeysMenu;
	configureKeysMenu->action = configureKeysMenuAction;
	
	audioOptionsMenu->draw = drawAudioOptionsMenu;
	audioOptionsMenu->action = audioOptionsMenuAction;
	
	audioEffectsOptionsMenu->draw = drawAudioEffectsOptionsMenu;
	audioEffectsOptionsMenu->action = audioEffectsOptionsMenuAction;
	
	audioMusicOptionsMenu->draw = drawAudioMusicOptionsMenu;
	audioMusicOptionsMenu->action = audioMusicOptionsMenuAction;
	
	quitMenu->draw = drawQuitMenu;
	quitMenu->action = quitMenuAction;
	
	// character config menu keys
	
	// pinkBubbleGum configs
	characterConfigureKeys[0].draw = drawPinkBubbleGumConfigKey;
	characterConfigureKeys[0].action = pinkBubbleGumKeyMenuAction;
	
	characterConfigureKeys[1].draw = drawPinkBubbleGumConfigRightKey;
	characterConfigureKeys[1].action = pinkBubbleGumRightKeyMenuAction;
	
	characterConfigureKeys[2].draw = drawPinkBubbleGumConfigLeftKey;
	characterConfigureKeys[2].action = pinkBubbleGumLeftKeyMenuAction;
	
	characterConfigureKeys[3].draw = drawPinkBubbleGumConfigUpKey;
	characterConfigureKeys[3].action = pinkBubbleGumUpKeyMenuAction;
	
	characterConfigureKeys[4].draw = drawPinkBubbleGumConfigDownKey;
	characterConfigureKeys[4].action = pinkBubbleGumDownKeyMenuAction;
	
	characterConfigureKeys[5].draw = drawPinkBubbleGumConfigFireKey;
	characterConfigureKeys[5].action = pinkBubbleGumFireKeyMenuAction;
	
	// redRover configs
	characterConfigureKeys[6].draw = drawRedRoverConfigKey;
	characterConfigureKeys[6].action = redRoverKeyMenuAction;
	
	characterConfigureKeys[7].draw = drawRedRoverConfigRightKey;
	characterConfigureKeys[7].action = redRoverRightKeyMenuAction;
	
	characterConfigureKeys[8].draw = drawRedRoverConfigLeftKey;
	characterConfigureKeys[8].action = redRoverLeftKeyMenuAction;
	
	characterConfigureKeys[9].draw = drawRedRoverConfigUpKey;
	characterConfigureKeys[9].action = redRoverUpKeyMenuAction;
	
	characterConfigureKeys[10].draw = drawRedRoverConfigDownKey;
	characterConfigureKeys[10].action = redRoverDownKeyMenuAction;
	
	characterConfigureKeys[11].draw = drawRedRoverConfigFireKey;
	characterConfigureKeys[11].action = redRoverFireKeyMenuAction;
	
	// GreenTree configs
	characterConfigureKeys[12].draw = drawGreenTreeConfigKey;
	characterConfigureKeys[12].action = greenTreeKeyMenuAction;
	
	characterConfigureKeys[13].draw = drawGreenTreeConfigRightKey;
	characterConfigureKeys[13].action = greenTreeRightKeyMenuAction;
	
	characterConfigureKeys[14].draw = drawGreenTreeConfigLeftKey;
	characterConfigureKeys[14].action = greenTreeLeftKeyMenuAction;
	
	characterConfigureKeys[15].draw = drawGreenTreeConfigUpKey;
	characterConfigureKeys[15].action = greenTreeUpKeyMenuAction;
	
	characterConfigureKeys[16].draw = drawGreenTreeConfigDownKey;
	characterConfigureKeys[16].action = greenTreeDownKeyMenuAction;
	
	characterConfigureKeys[17].draw = drawGreenTreeConfigFireKey;
	characterConfigureKeys[17].action = greenTreeFireKeyMenuAction;
	
	// blueLightning configs
	characterConfigureKeys[18].draw = drawBlueLightningConfigKey;
	characterConfigureKeys[18].action = blueLightningKeyMenuAction;
	
	characterConfigureKeys[19].draw = drawBlueLightningConfigRightKey;
	characterConfigureKeys[19].action = blueLightningRightKeyMenuAction;
	
	characterConfigureKeys[20].draw = drawBlueLightningConfigLeftKey;
	characterConfigureKeys[20].action = blueLightningLeftKeyMenuAction;
	
	characterConfigureKeys[21].draw = drawBlueLightningConfigUpKey;
	characterConfigureKeys[21].action = blueLightningUpKeyMenuAction;
	
	characterConfigureKeys[22].draw = drawBlueLightningConfigDownKey;
	characterConfigureKeys[22].action = blueLightningDownKeyMenuAction;
	
	characterConfigureKeys[23].draw = drawBlueLightningConfigFireKey;
	characterConfigureKeys[23].action = blueLightningFireKeyMenuAction;
		
	// Add Menus
	addSubMenu(&gMainMenu, gPlayMenu);
	addSubMenu(&gMainMenu, networkPlayMenu);
	addSubMenu(&gMainMenu, gameOptionsMenu);
	addSubMenu(&gMainMenu, quitMenu);
	
	addSubMenu(networkPlayMenu, networkServerMenu);
	addSubMenu(networkPlayMenu, networkClientMenu);
	addSubMenu(networkPlayMenu, networkUserNameMenu);
	
	addSubMenu(networkServerMenu, gNetworkServerPlayMenu);
	addSubMenu(networkServerMenu, networkServerNumberOfPlayersMenu);
	
	addSubMenu(networkClientMenu, networkAddressFieldMenu);
	addSubMenu(networkClientMenu, gConnectToNetworkGameMenu);
	
	addSubMenu(gameOptionsMenu, playerOptionsMenu);
	addSubMenu(playerOptionsMenu, pinkBubbleGumPlayerOptionsMenu);
	addSubMenu(playerOptionsMenu, gRedRoverPlayerOptionsMenu);
	addSubMenu(playerOptionsMenu, gGreenTreePlayerOptionsMenu);
	addSubMenu(playerOptionsMenu, gBlueLightningPlayerOptionsMenu);
	addSubMenu(playerOptionsMenu, gAIModeOptionsMenu);
	addSubMenu(playerOptionsMenu, gConfigureLivesMenu);
	
	addSubMenu(gameOptionsMenu, configureKeysMenu);
	addSubMenu(gameOptionsMenu, audioOptionsMenu);
	
	addSubMenu(audioOptionsMenu, audioEffectsOptionsMenu);
	addSubMenu(audioOptionsMenu, audioMusicOptionsMenu);
	
	// Configure keys submenus
	for (int characterIndex = 0; characterIndex < 4; characterIndex++)
	{
		addSubMenu(configureKeysMenu, &characterConfigureKeys[characterIndex * 6]);
		
		for (int submenuIndex = 1; submenuIndex < 6; submenuIndex++)
		{
			addSubMenu(&characterConfigureKeys[characterIndex * 6], &characterConfigureKeys[characterIndex * 6 + submenuIndex]);
		}
	}
}

void showPauseMenu(ZGWindow *window, GameState *gameState)
{
	changeMenu(RIGHT);
	pauseMusic();
	*gameState = GAME_STATE_PAUSED;
}

static char *convertKeyCodeToString(uint32_t theKeyCode)
{
	static char gKeyCode[64];
	const char *name = ZGGetKeyCodeName((uint16_t)theKeyCode);
	if (name == NULL || strlen(name) == 0)
	{
		snprintf(gKeyCode, sizeof(gKeyCode) - 1, "keycode %d", theKeyCode);
	}
	else
	{
		snprintf(gKeyCode, sizeof(gKeyCode) - 1, "%s", name);
		for (uint8_t index = 0; index < sizeof(gKeyCode); index++)
		{
			if (gKeyCode[index] == 0)
			{
				break;
			}
			gKeyCode[index] = tolower(gKeyCode[index]);
		}
	}
	
	return gKeyCode;
}

void setPendingKeyCode(uint32_t code)
{
	if (!ZGTestReturnKeyCode((uint16_t)code))
	{
		if (code != ZG_KEYCODE_ESCAPE)
		{
			*gMenuPendingKeyCode = code;
		}
		gMenuPendingOnKeyCode = false;
	}
}

void configureKey(uint32_t *id)
{
	gMenuPendingKeyCode = id;
	gMenuPendingOnKeyCode = true;
}

static void writeMenuTextInput(const char *text, size_t maxSize)
{
	if (gNetworkAddressFieldIsActive || gNetworkUserNameFieldIsActive)
	{
		for (uint8_t textIndex = 0; textIndex < maxSize; textIndex++)
		{
			if (text[textIndex] == 0x0 || text[textIndex] == 0x1)
			{
				break;
			}
			else if (gNetworkAddressFieldIsActive)
			{
				writeNetworkAddressText((uint8_t)text[textIndex]);
			}
			else if (gNetworkUserNameFieldIsActive)
			{
				writeNetworkUserNameText((uint8_t)text[textIndex]);
			}
		}
	}
}

static void performMenuEnterAction(GameState *gameState, ZGWindow *window)
{
	if (gCurrentMenu == gConfigureLivesMenu)
	{
		gDrawArrowsForCharacterLivesFlag = !gDrawArrowsForCharacterLivesFlag;
		if (gAudioEffectsFlag)
		{
			playMenuSound();
		}
	}

	else if (gCurrentMenu == gAIModeOptionsMenu)
	{
		gDrawArrowsForAIModeFlag = !gDrawArrowsForAIModeFlag;
		if (gAudioEffectsFlag)
		{
			playMenuSound();
		}
	}
	else
	{
		GameMenuContext menuContext;
		menuContext.gameState = gameState;
		menuContext.window = window;
		menuContext.exitGame = gExitGame;
		
		invokeMenu(&menuContext);
	}
}

static void performMenuDownAction(void)
{
	if (gNetworkAddressFieldIsActive)
		return;

	if (gDrawArrowsForCharacterLivesFlag)
	{
		if (gCharacterLives == 1)
		{
			gCharacterLives = MAX_CHARACTER_LIVES;
		}
		else
		{
			gCharacterLives--;
		}
	}
	else if (gDrawArrowsForAIModeFlag)
	{
		if (gAIMode == AI_EASY_MODE)
			gAIMode = AI_HARD_MODE;
		else if (gAIMode == AI_MEDIUM_MODE)
			gAIMode = AI_EASY_MODE;
		else /* if (gAIMode == AI_HARD_MODE) */
			gAIMode = AI_MEDIUM_MODE;
	}
	else if (gDrawArrowsForNumberOfNetHumansFlag)
	{
		gNumberOfNetHumans--;
		if (gNumberOfNetHumans <= 0)
		{
			gNumberOfNetHumans = 3;
		}
	}
	else
	{
		changeMenu(DOWN);

		if (gCurrentMenu == gRedRoverPlayerOptionsMenu && gPinkBubbleGum.state == CHARACTER_AI_STATE)
		{
			changeMenu(DOWN);
		}

		if (gCurrentMenu == gGreenTreePlayerOptionsMenu && gRedRover.state == CHARACTER_AI_STATE)
		{
			changeMenu(DOWN);
		}

		if (gCurrentMenu == gBlueLightningPlayerOptionsMenu && gGreenTree.state == CHARACTER_AI_STATE)
		{
			changeMenu(DOWN);
		}
	}
}

static void performMenuUpAction(void)
{
	if (gNetworkAddressFieldIsActive)
		return;

	if (gDrawArrowsForCharacterLivesFlag)
	{
		if (gCharacterLives == 10)
		{
			gCharacterLives = 1;
		}
		else
		{
			gCharacterLives++;
		}
	}
	else if (gDrawArrowsForAIModeFlag)
	{
		if (gAIMode == AI_EASY_MODE)
			gAIMode = AI_MEDIUM_MODE;
		else if (gAIMode == AI_MEDIUM_MODE)
			gAIMode = AI_HARD_MODE;
		else /* if (gAIMode == AI_HARD_MODE) */
			gAIMode = AI_EASY_MODE;
	}
	else if (gDrawArrowsForNumberOfNetHumansFlag)
	{
		gNumberOfNetHumans++;
		if (gNumberOfNetHumans >= 4)
		{
			gNumberOfNetHumans = 1;
		}
	}
	else
	{
		changeMenu(UP);

		if (gCurrentMenu == gBlueLightningPlayerOptionsMenu && gGreenTree.state == CHARACTER_AI_STATE)
		{
			changeMenu(UP);
		}

		if (gCurrentMenu == gGreenTreePlayerOptionsMenu && gRedRover.state == CHARACTER_AI_STATE)
		{
			changeMenu(UP);
		}

		if (gCurrentMenu == gRedRoverPlayerOptionsMenu && gPinkBubbleGum.state == CHARACTER_AI_STATE)
		{
			changeMenu(UP);
		}
	}
}

static void performMenuBackAction(GameState *gameState)
{
	if (*gameState == GAME_STATE_PAUSED)
	{
		changeMenu(LEFT);
		unPauseMusic();
		*gameState = GAME_STATE_ON;
	}
	else if (gNetworkAddressFieldIsActive)
	{
		gNetworkAddressFieldIsActive = false;
	}
	else if (gNetworkUserNameFieldIsActive)
	{
		gNetworkUserNameFieldIsActive = false;
	}
	else if (gDrawArrowsForCharacterLivesFlag)
	{
		gDrawArrowsForCharacterLivesFlag = false;
	}
	else if (gDrawArrowsForAIModeFlag)
	{
		gDrawArrowsForAIModeFlag = false;
	}
	else if (gDrawArrowsForNumberOfNetHumansFlag)
	{
		gDrawArrowsForNumberOfNetHumansFlag = false;
	}
	else
	{
		changeMenu(LEFT);
	}
}

void performKeyboardMenuAction(ZGKeyboardEvent *event, GameState *gameState, ZGWindow *window)
{
	uint16_t keyCode = event->keyCode;
	uint64_t keyModifier = event->keyModifier;
	
	if (gMenuPendingOnKeyCode)
	{
		setPendingKeyCode(keyCode);
	}
	else if (keyCode == ZG_KEYCODE_V && ZGTestMetaModifier(keyModifier))
	{
		char *clipboardText = ZGGetClipboardText();
		if (clipboardText != NULL)
		{
			writeMenuTextInput(clipboardText, 128);
			ZGFreeClipboardText(clipboardText);
		}
	}
	else if (ZGTestReturnKeyCode(keyCode))
	{
		performMenuEnterAction(gameState, window);
	}
	else if (keyCode == ZG_KEYCODE_DOWN)
	{
		performMenuDownAction();
	}
	else if (keyCode == ZG_KEYCODE_UP)
	{
		performMenuUpAction();
	}
	else if (keyCode == ZG_KEYCODE_ESCAPE)
	{
		performMenuBackAction(gameState);
	}
	else if (keyCode == ZG_KEYCODE_BACKSPACE)
	{
		if (gNetworkAddressFieldIsActive)
		{
			performNetworkAddressBackspace();
		}
		else if (gNetworkUserNameFieldIsActive)
		{
			performNetworkUserNameBackspace();
		}
	}
}

void performKeyboardMenuTextInputAction(ZGKeyboardEvent *event)
{
	writeMenuTextInput(event->text, sizeof(event->text));
}

void performGamepadMenuAction(GamepadEvent *event, GameState *gameState, ZGWindow *window)
{
	if (event->state != GAMEPAD_STATE_PRESSED)
	{
		return;
	}
	
	switch (event->button)
	{
		case GAMEPAD_BUTTON_A:
		case GAMEPAD_BUTTON_START:
			performMenuEnterAction(gameState, window);
			break;
		case GAMEPAD_BUTTON_B:
		case GAMEPAD_BUTTON_BACK:
			performMenuBackAction(gameState);
			break;
		case GAMEPAD_BUTTON_DPAD_UP:
			performMenuUpAction();
			break;
		case GAMEPAD_BUTTON_DPAD_DOWN:
			performMenuDownAction();
			break;
		case GAMEPAD_BUTTON_DPAD_LEFT:
		case GAMEPAD_BUTTON_DPAD_RIGHT:
		case GAMEPAD_BUTTON_X:
		case GAMEPAD_BUTTON_Y:
		case GAMEPAD_BUTTON_LEFTSHOULDER:
		case GAMEPAD_BUTTON_RIGHTSHOULDER:
		case GAMEPAD_BUTTON_LEFTTRIGGER:
		case GAMEPAD_BUTTON_RIGHTTRIGGER:
		case GAMEPAD_BUTTON_MAX:
			break;
	}
}
