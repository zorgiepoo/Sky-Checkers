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

static bool isChildBeingDrawn(Menu *child);

static void invokeMenu(void *context);
static void changeMenu(int direction);

static Menu *gConfigureLivesMenu;

static Menu *gAIModeOptionsMenu;
static Menu *gAINetModeOptionsMenu;

static Menu *gRedRoverPlayerOptionsMenu;
static Menu *gGreenTreePlayerOptionsMenu;
static Menu *gBlueLightningPlayerOptionsMenu;

static bool gDrawArrowsForCharacterLivesFlag;
static bool gDrawArrowsForAIModeFlag;
static bool gDrawArrowsForNumberOfNetHumansFlag;
static bool gDrawArrowsForNetPlayerLivesFlag;

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

#pragma mark Menu

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

static bool isChildBeingDrawn(Menu *child)
{
	return child->parent == gCurrentMenu->parent;
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

#pragma mark Game menus

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
	
	drawString(renderer, modelViewMatrix, preferredColor, 12.0f / 14.0f, 5.0f / 14.0f, "Play");
}

void playGameAction(void *context)
{
	GameMenuContext *menuContext = context;
	
	initGame(menuContext->window, true);
}

void drawNetworkPlayMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Online Play");
}

void networkPlayMenuAction(void *context)
{
	changeMenu(RIGHT);
}

void drawNetworkUserNameFieldMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t nameLabelModelViewMatrix = m4_translation((vec3_t){-0.36f, -1.07f, -20.00f});	
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
	drawString(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Server");
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
	if (gNetworkConnection != NULL && gNetworkConnection->thread != NULL)
	{
		fprintf(stderr, "game_menus: (server play) thread hasn't terminated yet.. Try again later.\n");
		return;
	}
	
	initializeNetwork();
	
	gNetworkConnection = malloc(sizeof(*gNetworkConnection));
	memset(gNetworkConnection, 0, sizeof(*gNetworkConnection));
	
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	
	struct addrinfo *serverInfoList;
	
	int getaddrinfoError;
	if ((getaddrinfoError = getaddrinfo(NULL, NETWORK_PORT, &hints, &serverInfoList)) != 0)
	{
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(getaddrinfoError));
		
		free(gNetworkConnection);
		gNetworkConnection = NULL;
		
		deinitializeNetwork();
		
		return;
	}
	
	struct addrinfo *serverInfo;
	for (serverInfo = serverInfoList; serverInfo != NULL; serverInfo = serverInfo->ai_next)
	{
		if (serverInfo->ai_family != AF_INET && serverInfo->ai_family != AF_INET6)
		{
			continue;
		}
		
		gNetworkConnection->socket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
		if (gNetworkConnection->socket == -1)
		{
			perror("server: socket");
			continue;
		}
		
#ifdef WINDOWS
		int addressLength = (int)serverInfo->ai_addrlen;
#else
		socklen_t addressLength = serverInfo->ai_addrlen;
#endif
		if (bind(gNetworkConnection->socket, serverInfo->ai_addr, addressLength) == -1)
		{
			perror("server: bind");
			closeSocket(gNetworkConnection->socket);
			continue;
		}
		
		break;
	}
	
	if (serverInfo == NULL)
	{
		fprintf(stderr, "Failed to create binding socket for server.\n");
		
		freeaddrinfo(serverInfoList);
		
		free(gNetworkConnection);
		gNetworkConnection = NULL;
		
		deinitializeNetwork();
		
		return;
	}
	
	freeaddrinfo(serverInfoList);
	
	gPinkBubbleGum.backup_state = gPinkBubbleGum.state;
	gRedRover.backup_state = gRedRover.state;
	gGreenTree.backup_state = gGreenTree.state;
	gBlueLightning.backup_state = gBlueLightning.state;
	
	gPinkBubbleGum.state = CHARACTER_HUMAN_STATE;
	gRedRover.state = CHARACTER_HUMAN_STATE;
	gGreenTree.state = gNumberOfNetHumans > 1 ? CHARACTER_HUMAN_STATE : CHARACTER_AI_STATE;
	gBlueLightning.state = gNumberOfNetHumans > 2 ? CHARACTER_HUMAN_STATE : CHARACTER_AI_STATE;
	
	gPinkBubbleGum.netState = NETWORK_PLAYING_STATE;
	gRedRover.netState = gRedRover.state == CHARACTER_HUMAN_STATE ? NETWORK_PENDING_STATE : NETWORK_PLAYING_STATE;
	gGreenTree.netState = gGreenTree.state == CHARACTER_HUMAN_STATE ? NETWORK_PENDING_STATE : NETWORK_PLAYING_STATE;
	gBlueLightning.netState = gBlueLightning.state == CHARACTER_HUMAN_STATE ? NETWORK_PENDING_STATE : NETWORK_PLAYING_STATE;
	
	gNetworkConnection->type = NETWORK_SERVER_TYPE;
	gNetworkConnection->character = &gPinkBubbleGum;
	gPinkBubbleGum.netName = gUserNameString;
	
	gNetworkConnection->numberOfPlayersToWaitFor = 0;
	gNetworkConnection->numberOfPlayersToWaitFor += (gRedRover.netState == NETWORK_PENDING_STATE);
	gNetworkConnection->numberOfPlayersToWaitFor += (gGreenTree.netState == NETWORK_PENDING_STATE);
	gNetworkConnection->numberOfPlayersToWaitFor += (gBlueLightning.netState == NETWORK_PENDING_STATE);
	
	gCurrentSlot = 0;
	memset(gClientStates, 0, sizeof(gClientStates));
	
	GameMenuContext *menuContext = context;
	initGame(menuContext->window, true);
	
	gRedRoverInput.character = gNetworkConnection->character;
	gBlueLightningInput.character = gNetworkConnection->character;
	gGreenTreeInput.character = gNetworkConnection->character;
	
	gNetworkConnection->thread = ZGCreateThread(serverNetworkThread, "server-thread", &gNetworkConnection->numberOfPlayersToWaitFor);
}

void drawNetworkServerNumberOfPlayersMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t numberPlayersModelViewMatrix = m4_translation((vec3_t){-1.43f, 0.0f, -20.00f});
	drawStringf(renderer, numberPlayersModelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Net-Players: %d", gNumberOfNetHumans);
	
	if (gDrawArrowsForNumberOfNetHumansFlag)
	{
		drawUpAndDownArrowTriangles(renderer, m4_translation((vec3_t){0.35f / 1.25f, 0.1f / 1.25f, -25.0f / 1.25f}));
	}
}

void networkServerNumberOfPlayersMenuAction(void *context)
{
	gDrawArrowsForNumberOfNetHumansFlag = !gDrawArrowsForNumberOfNetHumansFlag;
}

void drawNetworkServerAIModeMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t botModeModelViewMatrix = m4_translation((vec3_t){-1.43f, -1.07f, -20.00f});
	if (gAINetMode == AI_EASY_MODE)
		drawString(renderer, botModeModelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Bot Mode: Easy");
	
	else if (gAINetMode == AI_MEDIUM_MODE)
		drawString(renderer, botModeModelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Bot Mode: Medium");
	
	else if (gAINetMode == AI_HARD_MODE)
		drawString(renderer, botModeModelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Bot Mode: Hard");
	
	if (gDrawArrowsForAIModeFlag)
	{
		drawUpAndDownArrowTriangles(renderer, m4_translation((vec3_t){0.35f / 1.25f, -1.3f / 1.25f, -25.0f / 1.25f}));
	}
}

void networkServerAIModeMenuAction(void *context)
{
}

void drawNetworkServerPlayerLivesMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t playerLivesModelViewMatrix = m4_translation((vec3_t){-1.43f, -2.14f, -20.00f});
	drawStringf(renderer, playerLivesModelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Player Lives: %i", gCharacterNetLives);
	
	if (gDrawArrowsForNetPlayerLivesFlag)
	{
		drawUpAndDownArrowTriangles(renderer, m4_translation((vec3_t){0.35f / 1.25f, -2.6f / 1.25f, -25.0f / 1.25f}));
	}
}

void networkServerPlayerLivesMenuAction(void *context)
{
	gDrawArrowsForNetPlayerLivesFlag = !gDrawArrowsForNetPlayerLivesFlag;
}

void drawNetworkClientMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Client");
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
	if (gNetworkConnection != NULL && gNetworkConnection->thread != NULL)
	{
		fprintf(stderr, "game_menus: (connect to server) thread hasn't terminated yet.. Try again later.\n");
		return;
	}
	
	initializeNetwork();
	
	gNetworkConnection = malloc(sizeof(*gNetworkConnection));
	memset(gNetworkConnection, 0, sizeof(*gNetworkConnection));
	gNetworkConnection->type = NETWORK_CLIENT_TYPE;
	
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	
	struct addrinfo *serverInfoList;
	
	int getaddrinfoError;
	if ((getaddrinfoError = getaddrinfo(gServerAddressString, NETWORK_PORT, &hints, &serverInfoList)) != 0)
	{
		fprintf(stderr, "getaddrinfo client error: %s\n", gai_strerror(getaddrinfoError));
		
		free(gNetworkConnection);
		gNetworkConnection = NULL;
		
		deinitializeNetwork();
		
		return;
	}
	
	struct addrinfo *serverInfo;
	for (serverInfo = serverInfoList; serverInfo != NULL; serverInfo = serverInfo->ai_next)
	{
		if (serverInfo->ai_family != AF_INET && serverInfo->ai_family != AF_INET6)
		{
			continue;
		}
		
		gNetworkConnection->socket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
		if (gNetworkConnection->socket == -1)
		{
			perror("client: socket");
			continue;
		}
		
		break;
	}
	
	if (serverInfo == NULL)
	{
		fprintf(stderr, "Failed to create connecting socket for client.\n");
		
		freeaddrinfo(serverInfoList);
		
		free(gNetworkConnection);
		gNetworkConnection = NULL;
		
		deinitializeNetwork();
		
		return;
	}
	
	memcpy(&gNetworkConnection->hostAddress.storage, serverInfo->ai_addr, serverInfo->ai_addrlen);
	
	freeaddrinfo(serverInfoList);
	
	GameMenuContext *menuContext = context;
	GameState *gameState = menuContext->gameState;
	*gameState = GAME_STATE_CONNECTING;
	
	gNetworkConnection->thread = ZGCreateThread(clientNetworkThread, "client-thread", NULL);
}

void drawGameOptionsMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -1.07f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Game Options");
}

void gameOptionsMenuAction(void *context)
{
	changeMenu(RIGHT);
}

void drawPlayerOptionsMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 1.07f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Configure Players");
}

void playerOptionsMenuAction(void *context)
{
	changeMenu(RIGHT);
}

void drawConfigureKeysMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Configure Keys");
}

void configureKeysMenuAction(void *context)
{
	changeMenu(RIGHT);
}

void drawPinkBubbleGumPlayerOptionsMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 1.07f, -20.00f});	
	if (gPinkBubbleGum.state == CHARACTER_HUMAN_STATE)
		drawString(renderer, modelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Pink Bubblegum: Human");
	else if (gPinkBubbleGum.state == CHARACTER_AI_STATE)
		drawString(renderer, modelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Pink Bubblegum: Bot");
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
		drawString(renderer, modelViewMatrix, color, 20.0f / 14.0f, 5.0f / 14.0f, "Red Rover: Human");
	}
	else
	{
		drawString(renderer, modelViewMatrix, color, 20.0f / 14.0f, 5.0f / 14.0f, "Red Rover: Bot");
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
		drawString(renderer, modelViewMatrix, color, 20.0f / 14.0f, 5.0f / 14.0f, "Green Tree: Human");
	}
	else if (gGreenTree.state == CHARACTER_AI_STATE)
	{
		drawString(renderer, modelViewMatrix, color, 20.0f / 14.0f, 5.0f / 14.0f, "Green Tree: Bot");
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
		drawString(renderer, modelViewMatrix, color, 20.0f / 14.0f, 5.0f / 14.0f, "Blue Lightning: Human");
	}
	else if (gBlueLightning.state == CHARACTER_AI_STATE)
	{
		drawString(renderer, modelViewMatrix, color, 20.0f / 14.0f, 5.0f / 14.0f, "Blue Lightning: Bot");
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
		drawString(renderer, modeModelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Bot Mode: Easy");
	
	else if (gAIMode == AI_MEDIUM_MODE)
		drawString(renderer, modeModelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Bot Mode: Medium");
	
	else if (gAIMode == AI_HARD_MODE)
		drawString(renderer, modeModelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Bot Mode: Hard");
	
	if (gDrawArrowsForAIModeFlag)
	{
		drawUpAndDownArrowTriangles(renderer, m4_translation((vec3_t){2.0f / 1.25f, -4.1f / 1.25f, -25.0f / 1.25f}));
	}
}

void drawConfigureLivesMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t livesModelViewMatrix = m4_translation((vec3_t){-0.07f, -4.29f, -20.00f});	
	drawStringf(renderer, livesModelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Player Lives: %i", gCharacterLives);
	
	if (gDrawArrowsForCharacterLivesFlag)
	{
		drawUpAndDownArrowTriangles(renderer, m4_translation((vec3_t){2.0f / 1.25f, -5.4f / 1.25f, -25.0f / 1.25f}));
	}
}

void drawPinkBubbleGumConfigKey(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 1.07f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Pink Bubblegum");
}

void pinkBubbleGumKeyMenuAction(void *context)
{
	changeMenu(RIGHT);
}

void drawRedRoverConfigKey(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Red Rover");
}

void redRoverKeyMenuAction(void *context)
{
	changeMenu(RIGHT);
}

void drawGreenTreeConfigKey(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -1.07f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Green Tree");
}

void greenTreeKeyMenuAction(void *context)
{
	changeMenu(RIGHT);
}

void drawBlueLightningConfigKey(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -2.14f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Blue Lightning");
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
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -2.14f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Audio Options");
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
	gAudioMusicFlag = !gAudioMusicFlag;
	if (!gAudioMusicFlag)
	{
		stopMusic();
	}
	else
	{
		GameMenuContext *menuContext = context;
		bool windowFocus = ZGWindowHasFocus(menuContext->window);
		playMainMenuMusic(!windowFocus);
	}
}

void drawQuitMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -3.21f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 12.0f / 14.0f, 5.0f / 14.0f, "Quit");
}

void quitMenuAction(void *context)
{
	ZGSendQuitEvent();
}

void initMenus(void)
{
	initMainMenu();
	
	Menu *playMenu =							malloc(sizeof(Menu));
	Menu *networkPlayMenu =						malloc(sizeof(Menu));
	Menu *networkServerMenu =					malloc(sizeof(Menu));
	Menu *networkServerPlayMenu =				malloc(sizeof(Menu));
	Menu *networkServerNumberOfPlayersMenu =	malloc(sizeof(Menu));
	Menu *networkServerAIModeMenu =				malloc(sizeof(Menu));
	Menu *networkServerPlayerLivesMenu =		malloc(sizeof(Menu));
	Menu *networkClientMenu =					malloc(sizeof(Menu));
	Menu *networkUserNameMenu =					malloc(sizeof(Menu));
	Menu *networkAddressFieldMenu =				malloc(sizeof(Menu));
	Menu *connectToNetworkGameMenu =			malloc(sizeof(Menu));
	Menu *gameOptionsMenu =						malloc(sizeof(Menu));
	Menu *playerOptionsMenu =					malloc(sizeof(Menu));
	Menu *pinkBubbleGumPlayerOptionsMenu =		malloc(sizeof(Menu));
	gRedRoverPlayerOptionsMenu	=				malloc(sizeof(Menu));
	gGreenTreePlayerOptionsMenu =				malloc(sizeof(Menu));
	gBlueLightningPlayerOptionsMenu =			malloc(sizeof(Menu));
	gAIModeOptionsMenu =						malloc(sizeof(Menu));
	gConfigureLivesMenu =						malloc(sizeof(Menu));
	Menu *configureKeysMenu =					malloc(sizeof(Menu));
	// Four characters that each have their own menu + five configured menu actions (right, up, left, down, fire)
	Menu *characterConfigureKeys = 				malloc(sizeof(Menu) * 4 * 6);
	Menu *audioOptionsMenu =					malloc(sizeof(Menu));
	Menu *audioEffectsOptionsMenu =				malloc(sizeof(Menu));
	Menu *audioMusicOptionsMenu =				malloc(sizeof(Menu));
	Menu *quitMenu =							malloc(sizeof(Menu));
	
	// set action and drawing functions
	playMenu->draw = drawPlayMenu;
	playMenu->action = playGameAction;
	
	networkPlayMenu->draw = drawNetworkPlayMenu;
	networkPlayMenu->action = networkPlayMenuAction;
	
	networkServerMenu->draw = drawNetworkServerMenu;
	networkServerMenu->action = networkServerMenuAction;
	
	networkServerPlayMenu->draw = drawNetworkServerPlayMenu;
	networkServerPlayMenu->action = networkServerPlayMenuAction;
	
	networkServerNumberOfPlayersMenu->draw = drawNetworkServerNumberOfPlayersMenu;
	networkServerNumberOfPlayersMenu->action = networkServerNumberOfPlayersMenuAction;
	
	networkServerAIModeMenu->draw = drawNetworkServerAIModeMenu;
	networkServerAIModeMenu->action = networkServerAIModeMenuAction;
	gAINetModeOptionsMenu = networkServerAIModeMenu;
	
	networkServerPlayerLivesMenu->draw = drawNetworkServerPlayerLivesMenu;
	networkServerPlayerLivesMenu->action = networkServerPlayerLivesMenuAction;
	
	networkClientMenu->draw = drawNetworkClientMenu;
	networkClientMenu->action = networkClientMenuAction;
	
	networkUserNameMenu->draw = drawNetworkUserNameFieldMenu;
	networkUserNameMenu->action = networkUserNameFieldMenuAction;
	
	networkAddressFieldMenu->draw = drawNetworkAddressFieldMenu;
	networkAddressFieldMenu->action = networkAddressFieldMenuAction;
	
	connectToNetworkGameMenu->draw = drawConnectToNetworkGameMenu;
	connectToNetworkGameMenu->action = connectToNetworkGameMenuAction;
	
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
	addSubMenu(&gMainMenu, playMenu);
	addSubMenu(&gMainMenu, networkPlayMenu);
	addSubMenu(&gMainMenu, gameOptionsMenu);
	addSubMenu(&gMainMenu, audioOptionsMenu);
	addSubMenu(&gMainMenu, quitMenu);
	
	addSubMenu(networkPlayMenu, networkServerMenu);
	addSubMenu(networkPlayMenu, networkClientMenu);
	addSubMenu(networkPlayMenu, networkUserNameMenu);
	
	addSubMenu(networkServerMenu, networkServerPlayMenu);
	addSubMenu(networkServerMenu, networkServerNumberOfPlayersMenu);
	addSubMenu(networkServerMenu, networkServerAIModeMenu);
	addSubMenu(networkServerMenu, networkServerPlayerLivesMenu);
	
	addSubMenu(networkClientMenu, networkAddressFieldMenu);
	addSubMenu(networkClientMenu, connectToNetworkGameMenu);
	
	addSubMenu(gameOptionsMenu, playerOptionsMenu);
	addSubMenu(playerOptionsMenu, pinkBubbleGumPlayerOptionsMenu);
	addSubMenu(playerOptionsMenu, gRedRoverPlayerOptionsMenu);
	addSubMenu(playerOptionsMenu, gGreenTreePlayerOptionsMenu);
	addSubMenu(playerOptionsMenu, gBlueLightningPlayerOptionsMenu);
	addSubMenu(playerOptionsMenu, gAIModeOptionsMenu);
	addSubMenu(playerOptionsMenu, gConfigureLivesMenu);
	
	addSubMenu(gameOptionsMenu, configureKeysMenu);
	
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
		if (gCurrentMenu == gConfigureLivesMenu)
		{
			gDrawArrowsForCharacterLivesFlag = !gDrawArrowsForCharacterLivesFlag;
			if (gAudioEffectsFlag)
			{
				playMenuSound();
			}
		}

		else if (gCurrentMenu == gAIModeOptionsMenu || gCurrentMenu == gAINetModeOptionsMenu)
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
			
			invokeMenu(&menuContext);
		}
	}
	else if (keyCode == ZG_KEYCODE_DOWN)
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
		else if (gDrawArrowsForNetPlayerLivesFlag)
		{
			if (gCharacterNetLives == 1)
			{
				gCharacterNetLives = MAX_CHARACTER_LIVES;
			}
			else
			{
				gCharacterNetLives--;
			}
		}
		else if (gDrawArrowsForAIModeFlag)
		{
			if (isChildBeingDrawn(gAIModeOptionsMenu))
			{
				if (gAIMode == AI_EASY_MODE)
					gAIMode = AI_HARD_MODE;
				else if (gAIMode == AI_MEDIUM_MODE)
					gAIMode = AI_EASY_MODE;
				else /* if (gAIMode == AI_HARD_MODE) */
					gAIMode = AI_MEDIUM_MODE;
			}
			else if (isChildBeingDrawn(gAINetModeOptionsMenu))
			{
				if (gAINetMode == AI_EASY_MODE)
					gAINetMode = AI_HARD_MODE;
				else if (gAINetMode == AI_MEDIUM_MODE)
					gAINetMode = AI_EASY_MODE;
				else /* if (gAINetMode == AI_HARD_MODE) */
					gAINetMode = AI_MEDIUM_MODE;
			}
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
	else if (keyCode == ZG_KEYCODE_UP)
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
		else if (gDrawArrowsForNetPlayerLivesFlag)
		{
			if (gCharacterNetLives == 10)
			{
				gCharacterNetLives = 1;
			}
			else
			{
				gCharacterNetLives++;
			}
		}
		else if (gDrawArrowsForAIModeFlag)
		{
			if (isChildBeingDrawn(gAIModeOptionsMenu))
			{
				if (gAIMode == AI_EASY_MODE)
					gAIMode = AI_MEDIUM_MODE;
				else if (gAIMode == AI_MEDIUM_MODE)
					gAIMode = AI_HARD_MODE;
				else /* if (gAIMode == AI_HARD_MODE) */
					gAIMode = AI_EASY_MODE;
			}
			else if (isChildBeingDrawn(gAINetModeOptionsMenu))
			{
				if (gAINetMode == AI_EASY_MODE)
					gAINetMode = AI_MEDIUM_MODE;
				else if (gAINetMode == AI_MEDIUM_MODE)
					gAINetMode = AI_HARD_MODE;
				else /* if (gAINetMode == AI_HARD_MODE) */
					gAINetMode = AI_EASY_MODE;
			}
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
	else if (keyCode == ZG_KEYCODE_ESCAPE)
	{
		if (gNetworkAddressFieldIsActive)
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
		else if (gDrawArrowsForNetPlayerLivesFlag)
		{
			gDrawArrowsForNetPlayerLivesFlag = false;
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
