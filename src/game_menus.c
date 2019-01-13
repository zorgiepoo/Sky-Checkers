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

#include "game_menus.h"
#include "input.h"
#include "font.h"
#include "network.h"
#include "audio.h"
#include "utilities.h" // for SDL_Terminate()
#include "math_3d.h"
#include "renderer.h"

// Four characters that each have five configured menu actions (right, up, left, down, fire/slicing knife)
Menu gCharacterConfigureKeys[4][6];
Menu gJoyStickConfig[4][6];

Menu *gConfigureLivesMenu;
Menu *gScreenResolutionVideoOptionMenu;
Menu *gRefreshRateVideoOptionMenu;

Menu *gAIModeOptionsMenu;
Menu *gAINetModeOptionsMenu;

Menu *gRedRoverPlayerOptionsMenu;
Menu *gGreenTreePlayerOptionsMenu;
Menu *gBlueLightningPlayerOptionsMenu;

SDL_bool gDrawArrowsForCharacterLivesFlag =		SDL_FALSE;
SDL_bool gDrawArrowsForAIModeFlag =				SDL_FALSE;
SDL_bool gDrawArrowsForNumberOfNetHumansFlag =	SDL_FALSE;
SDL_bool gDrawArrowsForNetPlayerLivesFlag =		SDL_FALSE;

SDL_bool gNetworkAddressFieldIsActive =			SDL_FALSE;
SDL_bool gNetworkUserNameFieldIsActive =		SDL_FALSE;

// Variable that holds a keyCode for configuring keys (see convertKeyCodeToString() and menu configuration implementations)
static char gKeyCode[64];

char gServerAddressString[MAX_SERVER_ADDRESS_SIZE] = "127.0.0.1";
int gServerAddressStringIndex = 9;

char gUserNameString[MAX_USER_NAME_SIZE] = "Borg";
int gUserNameStringIndex = 4;

static char *convertKeyCodeToString(unsigned theKeyCode);
static unsigned getKey(void);
static unsigned getJoyStickTrigger(Sint16 *value, Uint8 *axis, int *joy_id);

static void drawUpAndDownArrowTriangles(Renderer *renderer, mat4_t modelViewMatrix)
{
	static BufferArrayObject vertexArrayObject;
	static SDL_bool initializedBuffer;
	if (!initializedBuffer)
	{
		float vertices[] =
		{
			0.0f, 0.6f, 0.0f,
			-0.2f, 0.2f, 0.0f,
			0.2f, 0.2f, 0.0f,
			
			0.0f, -0.6f, 0.0f,
			-0.2f, -0.2f, 0.0f,
			0.2f, -0.2f, 0.0f,
		};
		
		vertexArrayObject = createVertexArrayObject(renderer, vertices, sizeof(vertices));
		initializedBuffer = SDL_TRUE;
	}
	
	// Because all opaque objects should be rendered first, we will draw a transparent object
	// that matches all of the menu's
	drawVertices(renderer, modelViewMatrix, RENDERER_TRIANGLE_MODE, vertexArrayObject, 18, (color4_t){0.0f, 0.0f, 0.5f, 0.8f}, RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA);
}

/* A bunch of menu drawing and action functions! */

void drawPlayMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-1.0f / 14.0f, 15.0f / 14.0f, -20.0f});
	
	drawString(renderer, modelViewMatrix, preferredColor, 12.0f / 14.0f, 5.0f / 14.0f, "Play");
}

void playGameAction(void *context)
{
	initGame();
}

void drawNetworkPlayMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Network Play");
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

void writeNetworkUserNameText(Uint8 text)
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
	mat4_t modelViewMatrix = m4_translation((vec3_t){-1.43f, 0.00f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Start Game");
}

void networkServerPlayMenuAction(void *context)
{
	if (gNetworkConnection != NULL && gNetworkConnection->thread != NULL)
	{
		fprintf(stderr, "game_menus: (server play) thread hasn't terminated yet.. Try again later.\n");
		return;
	}
	
	networkInitialization();
	
	gNetworkConnection = malloc(sizeof(*gNetworkConnection));
	memset(gNetworkConnection, 0, sizeof(*gNetworkConnection));
	
	// open socket
	gNetworkConnection->socket = socket(AF_INET, SOCK_DGRAM, 0);
	
	if (gNetworkConnection->socket == -1)
	{
		perror("socket");
		
		free(gNetworkConnection);
		gNetworkConnection = NULL;
		
		return;
	}
	
	struct sockaddr_in address;
	
	// set address
	address.sin_family = AF_INET;
    address.sin_port = htons(NETWORK_PORT);
    address.sin_addr.s_addr = INADDR_ANY;
    memset(address.sin_zero, '\0', sizeof(address.sin_zero));
	
	// bind
	if (bind(gNetworkConnection->socket, (struct sockaddr *)&address, sizeof(address)) == -1)
	{
        perror("bind");
		
		closeSocket(gNetworkConnection->socket);
		free(gNetworkConnection);
		gNetworkConnection = NULL;
		
		return;
    }
	
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
	
	initGame();
	
	gRedRoverInput.character = gNetworkConnection->character;
	gBlueLightningInput.character = gNetworkConnection->character;
	gGreenTreeInput.character = gNetworkConnection->character;
	
	// make sure we are at main menu
	changeMenu(LEFT);
	changeMenu(LEFT);
	
	gNetworkConnection->thread = SDL_CreateThread(serverNetworkThread, "server-thread", &gNetworkConnection->numberOfPlayersToWaitFor);
}

void drawNetworkServerNumberOfPlayersMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t numberPlayersModelViewMatrix = m4_translation((vec3_t){-1.43f, -1.07f, -20.00f});	
	drawStringf(renderer, numberPlayersModelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Net-Players: %i", gNumberOfNetHumans);
	
	if (gDrawArrowsForNumberOfNetHumansFlag)
	{
		drawUpAndDownArrowTriangles(renderer, m4_translation((vec3_t){0.35f / 1.25f, -1.3f / 1.25f, -25.0f / 1.25f}));
	}
}

void networkServerNumberOfPlayersMenuAction(void *context)
{
	gDrawArrowsForNumberOfNetHumansFlag = !gDrawArrowsForNumberOfNetHumansFlag;
}

void drawNetworkServerAIModeMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t botModeModelViewMatrix = m4_translation((vec3_t){-1.43f, -2.14f, -20.00f});	
	if (gAINetMode == AI_EASY_MODE)
		drawString(renderer, botModeModelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Bot Mode: Easy");
	
	else if (gAINetMode == AI_MEDIUM_MODE)
		drawString(renderer, botModeModelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Bot Mode: Medium");
	
	else if (gAINetMode == AI_HARD_MODE)
		drawString(renderer, botModeModelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Bot Mode: Hard");
	
	if (gDrawArrowsForAIModeFlag)
	{
		drawUpAndDownArrowTriangles(renderer, m4_translation((vec3_t){0.35f / 1.25f, -2.7f / 1.25f, -25.0f / 1.25f}));
	}
}

void networkServerAIModeMenuAction(void *context)
{
}

void drawNetworkServerPlayerLivesMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t playerLivesModelViewMatrix = m4_translation((vec3_t){-1.43f, -3.21f, -20.00f});	
	drawStringf(renderer, playerLivesModelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Player Lives: %i", gCharacterNetLives);
	
	if (gDrawArrowsForNetPlayerLivesFlag)
	{
		drawUpAndDownArrowTriangles(renderer, m4_translation((vec3_t){0.35f / 1.25f, -4.0f / 1.25f, -25.0f / 1.25f}));
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
	mat4_t hostLabelModelViewMatrix = m4_translation((vec3_t){-2.86f, 0.00f, -20.00f});	
	drawString(renderer, hostLabelModelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Host Address:");
	
	color4_t color = gNetworkAddressFieldIsActive ? (color4_t){0.0f, 0.0f, 0.8f, 1.0f} : preferredColor;
	
	if (strlen(gServerAddressString) > 0)
	{
		mat4_t hostModelViewMatrix = m4_mul(hostLabelModelViewMatrix, m4_translation((vec3_t){1.6f, 0.0f, 0.0f}));
		
		drawStringLeftAligned(renderer, hostModelViewMatrix, color, 0.0024f, gServerAddressString);
	}
}

void networkAddressFieldMenuAction(void *context)
{
	gNetworkAddressFieldIsActive = !gNetworkAddressFieldIsActive;
}

void writeNetworkAddressText(Uint8 text)
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
	mat4_t modelViewMatrix = m4_translation((vec3_t){-3.21f, -1.07f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Connect");
}

void connectToNetworkGameMenuAction(void *context)
{
	if (gNetworkConnection != NULL && gNetworkConnection->thread != NULL)
	{
		fprintf(stderr, "game_menus: (connect to server) thread hasn't terminated yet.. Try again later.\n");
		return;
	}
	
	gNetworkConnection = malloc(sizeof(*gNetworkConnection));
	memset(gNetworkConnection, 0, sizeof(*gNetworkConnection));
	gNetworkConnection->type = NETWORK_CLIENT_TYPE;
	
	networkInitialization();
	
	// open socket
	gNetworkConnection->socket = socket(AF_INET, SOCK_DGRAM, 0);
	
	if (gNetworkConnection->socket == -1)
	{
		perror("socket");
		
		free(gNetworkConnection);
		gNetworkConnection = NULL;
		
		return;
	}
	
	struct hostent *host_entry = gethostbyname(gServerAddressString);
	
	if (host_entry == NULL)
	{
#ifndef WINDOWS
		herror("gethostbyname");
#else
		fprintf(stderr, "host_entry windows error\n");
#endif
		
		closeSocket(gNetworkConnection->socket);
		free(gNetworkConnection);
		gNetworkConnection = NULL;
		
		return;
	}
	
	// set address
	gNetworkConnection->hostAddress.sa_in.sin_family = AF_INET;
	gNetworkConnection->hostAddress.sa_in.sin_port = htons(NETWORK_PORT);
	gNetworkConnection->hostAddress.sa_in.sin_addr = *((struct in_addr *)host_entry->h_addr);
	memset(gNetworkConnection->hostAddress.sa_in.sin_zero, '\0', sizeof(gNetworkConnection->hostAddress.sa_in.sin_zero));
	
	gNetworkConnection->thread = SDL_CreateThread(clientNetworkThread, "client-thread", context);
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

void drawJoySticksConfigureMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -1.07f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Configure Joy Sticks");
}

void joySticksConfigureMenuAction(void *context)
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

void configureKey(unsigned *id)
{
	unsigned key = getKey();
	
	if (key != SDL_SCANCODE_UNKNOWN)
		*id = key;
}

// start configuration menus
void drawPinkBubbleGumConfigRightKey(Renderer *renderer, color4_t preferredColor)
{
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

void drawPinkBubbleGumConfigJoyStick(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 1.07f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Pink Bubblegum");
}

// config joy sticks.
void pinkBubbleGumConfigJoyStickAction(void *context)
{
	changeMenu(RIGHT);
}

void configureJoyStick(Input *input, int type)
{
	Sint16 value;
	Uint8 axis;
	int joy_id = 0;
	
	unsigned trigger = getJoyStickTrigger(&value, &axis, &joy_id);
	
	if (type == RIGHT)
	{
		input->joy_right_id = joy_id;
		
		if (value != (signed)JOY_NONE)
			input->rjs_id = value;
		
		input->rjs_axis_id = axis;
		
		if (axis != JOY_AXIS_NONE)
		{
			if (trigger == JOY_UP)
			{
				sprintf(input->joy_right, "Joy Up");
			} else if (trigger == JOY_RIGHT)
			{
				sprintf(input->joy_right, "Joy Right");
			} else if (trigger == JOY_DOWN)
			{
				sprintf(input->joy_right, "Joy Down");
			} else if (trigger == JOY_LEFT)
			{
				sprintf(input->joy_right, "Joy Left");
			}			
		}
		else
		{
			sprintf(input->joy_right, "Joy Button %i", input->rjs_id);
		}
	}
	
	else if (type == LEFT)
	{
		input->joy_left_id = joy_id;
		
		if (value != (signed)JOY_NONE)
			input->ljs_id = value;
		
		input->ljs_axis_id = axis;
		
		if (axis != JOY_AXIS_NONE)
		{
			if (trigger == JOY_UP)
			{
				sprintf(input->joy_left, "Joy Up");
			}
			else if (trigger == JOY_RIGHT)
			{
				sprintf(input->joy_left, "Joy Right");
			}
			else if (trigger == JOY_DOWN)
			{
				sprintf(input->joy_left, "Joy Down");
			}
			else if (trigger == JOY_LEFT)
			{
				sprintf(input->joy_left, "Joy Left");
			}			
		}
		else
		{
			sprintf(input->joy_left, "Joy Button %i", input->ljs_id);
		}
	}
	
	else if (type == UP)
	{
		input->joy_up_id = joy_id;
		
		if (value != (signed)JOY_NONE)
			input->ujs_id = value;
		
		input->ujs_axis_id = axis;
		
		if (axis != JOY_AXIS_NONE)
		{
			if (trigger == JOY_UP)
			{
				sprintf(input->joy_up, "Joy Up");
			}
			else if (trigger == JOY_RIGHT)
			{
				sprintf(input->joy_up, "Joy Right");
			}
			else if (trigger == JOY_DOWN)
			{
				sprintf(input->joy_up, "Joy Down");
			}
			else if (trigger == JOY_LEFT)
			{
				sprintf(input->joy_up, "Joy Left");
			}			
		}
		else
		{
			sprintf(input->joy_up, "Joy Button %i", input->ujs_id);
		}
	}
	
	else if (type == DOWN)
	{
		input->joy_down_id = joy_id;
		
		if (value != (signed)JOY_NONE)
			input->djs_id = value;
		
		input->djs_axis_id = axis;
		
		if (axis != JOY_AXIS_NONE)
		{
			if (trigger == JOY_UP)
			{
				sprintf(input->joy_down, "Joy Up");
			}
			else if (trigger == JOY_RIGHT)
			{
				sprintf(input->joy_down, "Joy Right");
			}
			else if (trigger == JOY_DOWN)
			{
				sprintf(input->joy_down, "Joy Down");
			}
			else if (trigger == JOY_LEFT)
			{
				sprintf(input->joy_down, "Joy Left");
			}			
		}
		else
		{
			sprintf(input->joy_down, "Joy Button %i", input->djs_id);
		}
		
	}
	else if (type == WEAPON)
	{
		input->joy_weap_id = joy_id;
		
		if (value != (signed)JOY_NONE)
			input->weapjs_id = value;
		
		input->weapjs_axis_id = axis;
		
		if (axis != JOY_AXIS_NONE)
		{
			if (trigger == JOY_UP)
			{
				sprintf(input->joy_weap, "Joy Up");
			}
			else if (trigger == JOY_RIGHT)
			{
				sprintf(input->joy_weap, "Joy Right");
			}
			else if (trigger == JOY_DOWN)
			{
				sprintf(input->joy_weap, "Joy Down");
			}
			else if (trigger == JOY_LEFT)
			{
				sprintf(input->joy_weap, "Joy Left");
			}			
		}
		else
		{
			sprintf(input->joy_weap, "Joy Button %i", input->weapjs_id);
		}
	}
}

void drawPinkBubbleGumRightgJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{	
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 1.07f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Right: %s", gPinkBubbleGumInput.joy_right);
}

void pinkBubbleGumConfigRightJoyStickAction(void *context)
{
	configureJoyStick(&gPinkBubbleGumInput, RIGHT);
}

void drawPinkBubbleGumLeftgJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Left: %s", gPinkBubbleGumInput.joy_left);
}

void pinkBubbleGumConfigLeftJoyStickAction(void *context)
{
	configureJoyStick(&gPinkBubbleGumInput, LEFT);
}

void drawPinkBubbleGumUpgJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -1.07f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Up: %s", gPinkBubbleGumInput.joy_up);
}

void pinkBubbleGumConfigUpJoyStickAction(void *context)
{
	configureJoyStick(&gPinkBubbleGumInput, UP);
}

void drawPinkBubbleGumDowngJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -2.14f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Down: %s", gPinkBubbleGumInput.joy_down);
}

void pinkBubbleGumConfigDownJoyStickAction(void *context)
{
	configureJoyStick(&gPinkBubbleGumInput, DOWN);
}

void drawPinkBubbleGumFiregJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -3.21f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Fire: %s", gPinkBubbleGumInput.joy_weap);
}

void pinkBubbleGumConfigFireJoyStickAction(void *context)
{
	configureJoyStick(&gPinkBubbleGumInput, WEAPON);
}

// RedRover
void drawRedRoverConfigJoyStick(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Red Rover");
}

void redRoverConfigJoyStickAction(void *context)
{
	changeMenu(RIGHT);
}

void drawRedRoverRightgJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{	
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 1.07f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Right: %s", gRedRoverInput.joy_right);
}

void redRoverConfigRightJoyStickAction(void *context)
{
	configureJoyStick(&gRedRoverInput, RIGHT);
}

void drawRedRoverLeftgJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Left: %s", gRedRoverInput.joy_left);
}

void redRoverConfigLeftJoyStickAction(void *context)
{
	configureJoyStick(&gRedRoverInput, LEFT);
}

void drawRedRoverUpgJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -1.07f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Up: %s", gRedRoverInput.joy_up);
}

void redRoverConfigUpJoyStickAction(void *context)
{
	configureJoyStick(&gRedRoverInput, UP);
}

void drawRedRoverDowngJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -2.14f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Down: %s", gRedRoverInput.joy_down);
}

void redRoverConfigDownJoyStickAction(void *context)
{
	configureJoyStick(&gRedRoverInput, DOWN);
}

void drawRedRoverFiregJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -3.21f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Fire: %s", gRedRoverInput.joy_weap);
}

void redRoverConfigFireJoyStickAction(void *context)
{
	configureJoyStick(&gRedRoverInput, WEAPON);
}

// GreenTree
void drawGreenTreeConfigJoyStick(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -1.07f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Green Tree");
}

void greenTreeConfigJoyStickAction(void *context)
{
	changeMenu(RIGHT);
}

void drawGreenTreeRightgJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{	
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 1.07f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Right: %s", gGreenTreeInput.joy_right);
}

void greenTreeConfigRightJoyStickAction(void *context)
{
	configureJoyStick(&gGreenTreeInput, RIGHT);
}

void drawGreenTreeLeftgJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Left: %s", gGreenTreeInput.joy_left);
}

void greenTreeConfigLeftJoyStickAction(void *context)
{
	configureJoyStick(&gGreenTreeInput, LEFT);
}

void drawGreenTreeUpgJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -1.07f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Up: %s", gGreenTreeInput.joy_up);
}

void greenTreeConfigUpJoyStickAction(void *context)
{
	configureJoyStick(&gGreenTreeInput, UP);
}

void drawGreenTreeDowngJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -2.14f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Down: %s", gGreenTreeInput.joy_down);
}

void greenTreeConfigDownJoyStickAction(void *context)
{
	configureJoyStick(&gGreenTreeInput, DOWN);
}

void drawGreenTreeFiregJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -3.21f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Fire: %s", gGreenTreeInput.joy_weap);
}

void greenTreeConfigFireJoyStickAction(void *context)
{
	configureJoyStick(&gGreenTreeInput, WEAPON);
}

// BlueLightning
void drawBlueLightningConfigJoyStick(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -2.14f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Blue Lightning");
}

void blueLightningConfigJoyStickAction(void *context)
{
	changeMenu(RIGHT);
}

void drawBlueLightningRightgJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{	
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 1.07f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Right: %s", gBlueLightningInput.joy_right);
}

void blueLightningConfigRightJoyStickAction(void *context)
{
	configureJoyStick(&gBlueLightningInput, RIGHT);
}

void drawBlueLightningLeftgJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Left: %s", gBlueLightningInput.joy_left);
}

void blueLightningConfigLeftJoyStickAction(void *context)
{
	configureJoyStick(&gBlueLightningInput, LEFT);
}

void drawBlueLightningUpgJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -1.07f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Up: %s", gBlueLightningInput.joy_up);
}

void blueLightningConfigUpJoyStickAction(void *context)
{
	configureJoyStick(&gBlueLightningInput, UP);
}

void drawBlueLightningDowngJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -2.14f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Down: %s", gBlueLightningInput.joy_down);
}

void blueLightningConfigDownJoyStickAction(void *context)
{
	configureJoyStick(&gBlueLightningInput, DOWN);
}

void drawBlueLightningFiregJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -3.21f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Fire: %s", gBlueLightningInput.joy_weap);
}

void blueLightningConfigFireJoyStickAction(void *context)
{
	configureJoyStick(&gBlueLightningInput, WEAPON);
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
}

void drawQuitMenu(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -3.21f, -20.00f});	
	drawString(renderer, modelViewMatrix, preferredColor, 12.0f / 14.0f, 5.0f / 14.0f, "Quit");
}

void quitMenuAction(void *context)
{
	SDL_Event event;
	event.type = SDL_QUIT;
	SDL_PushEvent(&event);
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
	Menu *configureJoySticksMenu =				malloc(sizeof(Menu));
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
	
	configureJoySticksMenu->draw = drawJoySticksConfigureMenu;
	configureJoySticksMenu->action = joySticksConfigureMenuAction;
	
	audioOptionsMenu->draw = drawAudioOptionsMenu;
	audioOptionsMenu->action = audioOptionsMenuAction;
	
	audioEffectsOptionsMenu->draw = drawAudioEffectsOptionsMenu;
	audioEffectsOptionsMenu->action = audioEffectsOptionsMenuAction;
	
	audioMusicOptionsMenu->draw = drawAudioMusicOptionsMenu;
	audioMusicOptionsMenu->action = audioMusicOptionsMenuAction;
	
	quitMenu->draw = drawQuitMenu;
	quitMenu->action = quitMenuAction;
	
	// character config menu keys
	gCharacterConfigureKeys[0][0].draw = drawPinkBubbleGumConfigKey;
	gCharacterConfigureKeys[0][0].action = pinkBubbleGumKeyMenuAction;
	
	gCharacterConfigureKeys[1][0].draw = drawRedRoverConfigKey;
	gCharacterConfigureKeys[1][0].action = redRoverKeyMenuAction;
	
	gCharacterConfigureKeys[2][0].draw = drawGreenTreeConfigKey;
	gCharacterConfigureKeys[2][0].action = greenTreeKeyMenuAction;
	
	gCharacterConfigureKeys[3][0].draw = drawBlueLightningConfigKey;
	gCharacterConfigureKeys[3][0].action = blueLightningKeyMenuAction;
	
	// pinkBubbleGum configs
	gCharacterConfigureKeys[0][1].draw = drawPinkBubbleGumConfigRightKey;
	gCharacterConfigureKeys[0][1].action = pinkBubbleGumRightKeyMenuAction;
	
	gCharacterConfigureKeys[0][2].draw = drawPinkBubbleGumConfigLeftKey;
	gCharacterConfigureKeys[0][2].action = pinkBubbleGumLeftKeyMenuAction;
	
	gCharacterConfigureKeys[0][3].draw = drawPinkBubbleGumConfigUpKey;
	gCharacterConfigureKeys[0][3].action = pinkBubbleGumUpKeyMenuAction;
	
	gCharacterConfigureKeys[0][4].draw = drawPinkBubbleGumConfigDownKey;
	gCharacterConfigureKeys[0][4].action = pinkBubbleGumDownKeyMenuAction;
	
	gCharacterConfigureKeys[0][5].draw = drawPinkBubbleGumConfigFireKey;
	gCharacterConfigureKeys[0][5].action = pinkBubbleGumFireKeyMenuAction;
	
	// redRover configs
	gCharacterConfigureKeys[1][1].draw = drawRedRoverConfigRightKey;
	gCharacterConfigureKeys[1][1].action = redRoverRightKeyMenuAction;
	
	gCharacterConfigureKeys[1][2].draw = drawRedRoverConfigLeftKey;
	gCharacterConfigureKeys[1][2].action = redRoverLeftKeyMenuAction;
	
	gCharacterConfigureKeys[1][3].draw = drawRedRoverConfigUpKey;
	gCharacterConfigureKeys[1][3].action = redRoverUpKeyMenuAction;
	
	gCharacterConfigureKeys[1][4].draw = drawRedRoverConfigDownKey;
	gCharacterConfigureKeys[1][4].action = redRoverDownKeyMenuAction;
	
	gCharacterConfigureKeys[1][5].draw = drawRedRoverConfigFireKey;
	gCharacterConfigureKeys[1][5].action = redRoverFireKeyMenuAction;
	
	// GreenTree configs
	gCharacterConfigureKeys[2][1].draw = drawGreenTreeConfigRightKey;
	gCharacterConfigureKeys[2][1].action = greenTreeRightKeyMenuAction;
	
	gCharacterConfigureKeys[2][2].draw = drawGreenTreeConfigLeftKey;
	gCharacterConfigureKeys[2][2].action = greenTreeLeftKeyMenuAction;
	
	gCharacterConfigureKeys[2][3].draw = drawGreenTreeConfigUpKey;
	gCharacterConfigureKeys[2][3].action = greenTreeUpKeyMenuAction;
	
	gCharacterConfigureKeys[2][4].draw = drawGreenTreeConfigDownKey;
	gCharacterConfigureKeys[2][4].action = greenTreeDownKeyMenuAction;
	
	gCharacterConfigureKeys[2][5].draw = drawGreenTreeConfigFireKey;
	gCharacterConfigureKeys[2][5].action = greenTreeFireKeyMenuAction;
	
	// blueLightning configs
	gCharacterConfigureKeys[3][1].draw = drawBlueLightningConfigRightKey;
	gCharacterConfigureKeys[3][1].action = blueLightningRightKeyMenuAction;
	
	gCharacterConfigureKeys[3][2].draw = drawBlueLightningConfigLeftKey;
	gCharacterConfigureKeys[3][2].action = blueLightningLeftKeyMenuAction;
	
	gCharacterConfigureKeys[3][3].draw = drawBlueLightningConfigUpKey;
	gCharacterConfigureKeys[3][3].action = blueLightningUpKeyMenuAction;
	
	gCharacterConfigureKeys[3][4].draw = drawBlueLightningConfigDownKey;
	gCharacterConfigureKeys[3][4].action = blueLightningDownKeyMenuAction;
	
	gCharacterConfigureKeys[3][5].draw = drawBlueLightningConfigFireKey;
	gCharacterConfigureKeys[3][5].action = blueLightningFireKeyMenuAction;
	
	// Joystick menus
	
	// PinkBubbleGum configs
	gJoyStickConfig[0][0].draw = drawPinkBubbleGumConfigJoyStick;
	gJoyStickConfig[0][0].action = pinkBubbleGumConfigJoyStickAction;
	
	gJoyStickConfig[0][1].draw = drawPinkBubbleGumRightgJoyStickConfig;
	gJoyStickConfig[0][1].action = pinkBubbleGumConfigRightJoyStickAction;
	
	gJoyStickConfig[0][2].draw = drawPinkBubbleGumLeftgJoyStickConfig;
	gJoyStickConfig[0][2].action = pinkBubbleGumConfigLeftJoyStickAction;
	
	gJoyStickConfig[0][3].draw = drawPinkBubbleGumUpgJoyStickConfig;
	gJoyStickConfig[0][3].action = pinkBubbleGumConfigUpJoyStickAction;
	
	gJoyStickConfig[0][4].draw = drawPinkBubbleGumDowngJoyStickConfig;
	gJoyStickConfig[0][4].action = pinkBubbleGumConfigDownJoyStickAction;
	
	gJoyStickConfig[0][5].draw = drawPinkBubbleGumFiregJoyStickConfig;
	gJoyStickConfig[0][5].action = pinkBubbleGumConfigFireJoyStickAction;
	
	// RedRover configs
	gJoyStickConfig[1][0].draw = drawRedRoverConfigJoyStick;
	gJoyStickConfig[1][0].action = redRoverConfigJoyStickAction;
	
	gJoyStickConfig[1][1].draw = drawRedRoverRightgJoyStickConfig;
	gJoyStickConfig[1][1].action = redRoverConfigRightJoyStickAction;
	
	gJoyStickConfig[1][2].draw = drawRedRoverLeftgJoyStickConfig;
	gJoyStickConfig[1][2].action = redRoverConfigLeftJoyStickAction;
	
	gJoyStickConfig[1][3].draw = drawRedRoverUpgJoyStickConfig;
	gJoyStickConfig[1][3].action = redRoverConfigUpJoyStickAction;
	
	gJoyStickConfig[1][4].draw = drawRedRoverDowngJoyStickConfig;
	gJoyStickConfig[1][4].action = redRoverConfigDownJoyStickAction;
	
	gJoyStickConfig[1][5].draw = drawRedRoverFiregJoyStickConfig;
	gJoyStickConfig[1][5].action = redRoverConfigFireJoyStickAction;
	
	// GreenTree configs
	gJoyStickConfig[2][0].draw = drawGreenTreeConfigJoyStick;
	gJoyStickConfig[2][0].action = greenTreeConfigJoyStickAction;
	
	gJoyStickConfig[2][1].draw = drawGreenTreeRightgJoyStickConfig;
	gJoyStickConfig[2][1].action = greenTreeConfigRightJoyStickAction;
	
	gJoyStickConfig[2][2].draw = drawGreenTreeLeftgJoyStickConfig;
	gJoyStickConfig[2][2].action = greenTreeConfigLeftJoyStickAction;
	
	gJoyStickConfig[2][3].draw = drawGreenTreeUpgJoyStickConfig;
	gJoyStickConfig[2][3].action = greenTreeConfigUpJoyStickAction;
	
	gJoyStickConfig[2][4].draw = drawGreenTreeDowngJoyStickConfig;
	gJoyStickConfig[2][4].action = greenTreeConfigDownJoyStickAction;
	
	gJoyStickConfig[2][5].draw = drawGreenTreeFiregJoyStickConfig;
	gJoyStickConfig[2][5].action = greenTreeConfigFireJoyStickAction;
	
	// BlueLightning configs
	gJoyStickConfig[3][0].draw = drawBlueLightningConfigJoyStick;
	gJoyStickConfig[3][0].action = blueLightningConfigJoyStickAction;
	
	gJoyStickConfig[3][1].draw = drawBlueLightningRightgJoyStickConfig;
	gJoyStickConfig[3][1].action = blueLightningConfigRightJoyStickAction;
	
	gJoyStickConfig[3][2].draw = drawBlueLightningLeftgJoyStickConfig;
	gJoyStickConfig[3][2].action = blueLightningConfigLeftJoyStickAction;
	
	gJoyStickConfig[3][3].draw = drawBlueLightningUpgJoyStickConfig;
	gJoyStickConfig[3][3].action = blueLightningConfigUpJoyStickAction;
	
	gJoyStickConfig[3][4].draw = drawBlueLightningDowngJoyStickConfig;
	gJoyStickConfig[3][4].action = blueLightningConfigDownJoyStickAction;
	
	gJoyStickConfig[3][5].draw = drawBlueLightningFiregJoyStickConfig;
	gJoyStickConfig[3][5].action = blueLightningConfigFireJoyStickAction;
	
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
	addSubMenu(gameOptionsMenu, configureJoySticksMenu);
	
	addSubMenu(audioOptionsMenu, audioEffectsOptionsMenu);
	addSubMenu(audioOptionsMenu, audioMusicOptionsMenu);
	
	// characters key config menu
	addSubMenu(configureKeysMenu, &gCharacterConfigureKeys[0][0]);
	addSubMenu(configureKeysMenu, &gCharacterConfigureKeys[1][0]);
	addSubMenu(configureKeysMenu, &gCharacterConfigureKeys[2][0]);
	addSubMenu(configureKeysMenu, &gCharacterConfigureKeys[3][0]);
	
	// characters joy stick config menu
	addSubMenu(configureJoySticksMenu, &gJoyStickConfig[0][0]);
	addSubMenu(configureJoySticksMenu, &gJoyStickConfig[1][0]);
	addSubMenu(configureJoySticksMenu, &gJoyStickConfig[2][0]);
	addSubMenu(configureJoySticksMenu, &gJoyStickConfig[3][0]);
	
	// keys config sub menus
	
	// pinkBubbleGum
	addSubMenu(&gCharacterConfigureKeys[0][0], &gCharacterConfigureKeys[0][1]);
	addSubMenu(&gCharacterConfigureKeys[0][0], &gCharacterConfigureKeys[0][2]);
	addSubMenu(&gCharacterConfigureKeys[0][0], &gCharacterConfigureKeys[0][3]);
	addSubMenu(&gCharacterConfigureKeys[0][0], &gCharacterConfigureKeys[0][4]);
	addSubMenu(&gCharacterConfigureKeys[0][0], &gCharacterConfigureKeys[0][5]);
	
	// redRover
	addSubMenu(&gCharacterConfigureKeys[1][0], &gCharacterConfigureKeys[1][1]);
	addSubMenu(&gCharacterConfigureKeys[1][0], &gCharacterConfigureKeys[1][2]);
	addSubMenu(&gCharacterConfigureKeys[1][0], &gCharacterConfigureKeys[1][3]);
	addSubMenu(&gCharacterConfigureKeys[1][0], &gCharacterConfigureKeys[1][4]);
	addSubMenu(&gCharacterConfigureKeys[1][0], &gCharacterConfigureKeys[1][5]);
	
	// greenTree
	addSubMenu(&gCharacterConfigureKeys[2][0], &gCharacterConfigureKeys[2][1]);
	addSubMenu(&gCharacterConfigureKeys[2][0], &gCharacterConfigureKeys[2][2]);
	addSubMenu(&gCharacterConfigureKeys[2][0], &gCharacterConfigureKeys[2][3]);
	addSubMenu(&gCharacterConfigureKeys[2][0], &gCharacterConfigureKeys[2][4]);
	addSubMenu(&gCharacterConfigureKeys[2][0], &gCharacterConfigureKeys[2][5]);
	
	// blueLightning
	addSubMenu(&gCharacterConfigureKeys[3][0], &gCharacterConfigureKeys[3][1]);
	addSubMenu(&gCharacterConfigureKeys[3][0], &gCharacterConfigureKeys[3][2]);
	addSubMenu(&gCharacterConfigureKeys[3][0], &gCharacterConfigureKeys[3][3]);
	addSubMenu(&gCharacterConfigureKeys[3][0], &gCharacterConfigureKeys[3][4]);
	addSubMenu(&gCharacterConfigureKeys[3][0], &gCharacterConfigureKeys[3][5]);
	
	// joy stick config sub menus
	
	// PinkBubbleGum
	addSubMenu(&gJoyStickConfig[0][0], &gJoyStickConfig[0][1]);
	addSubMenu(&gJoyStickConfig[0][0], &gJoyStickConfig[0][2]);
	addSubMenu(&gJoyStickConfig[0][0], &gJoyStickConfig[0][3]);
	addSubMenu(&gJoyStickConfig[0][0], &gJoyStickConfig[0][4]);
	addSubMenu(&gJoyStickConfig[0][0], &gJoyStickConfig[0][5]);
	
	// RedRover
	addSubMenu(&gJoyStickConfig[1][0], &gJoyStickConfig[1][1]);
	addSubMenu(&gJoyStickConfig[1][0], &gJoyStickConfig[1][2]);
	addSubMenu(&gJoyStickConfig[1][0], &gJoyStickConfig[1][3]);
	addSubMenu(&gJoyStickConfig[1][0], &gJoyStickConfig[1][4]);
	addSubMenu(&gJoyStickConfig[1][0], &gJoyStickConfig[1][5]);
	
	// GreenTree
	addSubMenu(&gJoyStickConfig[2][0], &gJoyStickConfig[2][1]);
	addSubMenu(&gJoyStickConfig[2][0], &gJoyStickConfig[2][2]);
	addSubMenu(&gJoyStickConfig[2][0], &gJoyStickConfig[2][3]);
	addSubMenu(&gJoyStickConfig[2][0], &gJoyStickConfig[2][4]);
	addSubMenu(&gJoyStickConfig[2][0], &gJoyStickConfig[2][5]);
	
	// BlueLightning
	addSubMenu(&gJoyStickConfig[3][0], &gJoyStickConfig[3][1]);
	addSubMenu(&gJoyStickConfig[3][0], &gJoyStickConfig[3][2]);
	addSubMenu(&gJoyStickConfig[3][0], &gJoyStickConfig[3][3]);
	addSubMenu(&gJoyStickConfig[3][0], &gJoyStickConfig[3][4]);
	addSubMenu(&gJoyStickConfig[3][0], &gJoyStickConfig[3][5]);
}

static char *convertKeyCodeToString(unsigned theKeyCode)
{
	switch (theKeyCode)
	{
		case SDL_SCANCODE_RIGHT:
			sprintf(gKeyCode, "right arrow");
			break;
		case SDL_SCANCODE_LEFT:
			sprintf(gKeyCode, "left arrow");
			break;
		case SDL_SCANCODE_UP:
			sprintf(gKeyCode, "up arrow");
			break;
		case SDL_SCANCODE_DOWN:
			sprintf(gKeyCode, "down arrow");
			break;
		case SDL_SCANCODE_SPACE:
			sprintf(gKeyCode, "spacebar");
			break;
		case SDL_SCANCODE_INSERT:
			sprintf(gKeyCode, "insert");
			break;
		case SDL_SCANCODE_HOME:
			sprintf(gKeyCode, "home");
			break;
		case SDL_SCANCODE_END:
			sprintf(gKeyCode, "end");
			break;
		case SDL_SCANCODE_PAGEUP:
			sprintf(gKeyCode, "pageup");
			break;
		case SDL_SCANCODE_PAGEDOWN:
			sprintf(gKeyCode, "pagedown");
			break;
		case SDL_SCANCODE_RSHIFT:
		case SDL_SCANCODE_LSHIFT:
			sprintf(gKeyCode, "shift");
			break;
		case SDL_SCANCODE_BACKSPACE:
			sprintf(gKeyCode, "backspace");
			break;
		case SDL_SCANCODE_TAB:
			sprintf(gKeyCode, "tab");
			break;
		case SDL_SCANCODE_F1:
			sprintf(gKeyCode, "F1");
			break;
		case SDL_SCANCODE_F2:
			sprintf(gKeyCode, "F2");
			break;
		case SDL_SCANCODE_F3:
			sprintf(gKeyCode, "F3");
			break;
		case SDL_SCANCODE_F4:
			sprintf(gKeyCode, "F4");
			break;
		case SDL_SCANCODE_F5:
			sprintf(gKeyCode, "F5");
			break;
		case SDL_SCANCODE_F6:
			sprintf(gKeyCode, "F6");
			break;
		case SDL_SCANCODE_F7:
			sprintf(gKeyCode, "F7");
			break;
		case SDL_SCANCODE_F8:
			sprintf(gKeyCode, "F8");
			break;
		case SDL_SCANCODE_F9:
			sprintf(gKeyCode, "F9");
			break;
		case SDL_SCANCODE_F10:
			sprintf(gKeyCode, "F10");
			break;
		case SDL_SCANCODE_F11:
			sprintf(gKeyCode, "F11");
			break;
		case SDL_SCANCODE_F12:
			sprintf(gKeyCode, "F12");
			break;
		case SDL_SCANCODE_F13:
			sprintf(gKeyCode, "F13");
			break;
		case SDL_SCANCODE_F14:
			sprintf(gKeyCode, "F14");
			break;
		case SDL_SCANCODE_F15:
			sprintf(gKeyCode, "F15");
			break;
		case SDL_SCANCODE_CAPSLOCK:
			sprintf(gKeyCode, "capslock");
			break;
		case SDL_SCANCODE_NUMLOCKCLEAR:
			sprintf(gKeyCode, "numlock");
			break;
		case SDL_SCANCODE_SCROLLLOCK:
			sprintf(gKeyCode, "scrollock");
			break;
		case SDL_SCANCODE_RCTRL:
		case SDL_SCANCODE_LCTRL:
			sprintf(gKeyCode, "control");
			break;
		case SDL_SCANCODE_RALT:
		case SDL_SCANCODE_LALT:
			sprintf(gKeyCode, "alt");
			break;
		case SDL_SCANCODE_RGUI:
		case SDL_SCANCODE_LGUI:
			sprintf(gKeyCode, "meta/command");
			break;
		case SDL_SCANCODE_KP_0:
			sprintf(gKeyCode, "keypad 0");
			break;
		case SDL_SCANCODE_KP_1:
			sprintf(gKeyCode, "keypad 1");
			break;
		case SDL_SCANCODE_KP_2:
			sprintf(gKeyCode, "keypad 2");
			break;
		case SDL_SCANCODE_KP_3:
			sprintf(gKeyCode, "keypad 3");
			break;
		case SDL_SCANCODE_KP_4:
			sprintf(gKeyCode, "keypad 4");
			break;
		case SDL_SCANCODE_KP_5:
			sprintf(gKeyCode, "keypad 5");
			break;
		case SDL_SCANCODE_KP_6:
			sprintf(gKeyCode, "keypad 6");
			break;
		case SDL_SCANCODE_KP_7:
			sprintf(gKeyCode, "keypad 7");
			break;
		case SDL_SCANCODE_KP_8:
			sprintf(gKeyCode, "keypad 8");
			break;
		case SDL_SCANCODE_KP_9:
			sprintf(gKeyCode, "keypad 9");
			break;
		case SDL_SCANCODE_KP_PERIOD:
			sprintf(gKeyCode, "keypad .");
			break;
		case SDL_SCANCODE_KP_DIVIDE:
			sprintf(gKeyCode, "keypad /");
			break;
		case SDL_SCANCODE_KP_MULTIPLY:
			sprintf(gKeyCode, "keypad *");
			break;
		case SDL_SCANCODE_KP_MINUS:
			sprintf(gKeyCode, "keypad -");
			break;
		case SDL_SCANCODE_KP_PLUS:
			sprintf(gKeyCode, "keypad +");
			break;
		case SDL_SCANCODE_KP_ENTER:
			sprintf(gKeyCode, "keypad enter");
			break;
		case SDL_SCANCODE_KP_EQUALS:
			sprintf(gKeyCode, "keypad =");
			break;
			
		default:
		{
			const char *name = SDL_GetScancodeName(theKeyCode);
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
		}
	}
	
	return gKeyCode;
}

static unsigned getKey(void)
{
	SDL_Event event;
	SDL_bool quit = SDL_FALSE;
	unsigned key = 0;
	
	while (!quit)
	{
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_KEYDOWN:
					
					if (event.key.keysym.scancode != SDL_SCANCODE_RETURN && event.key.keysym.scancode != SDL_SCANCODE_RETURN2 && event.key.keysym.scancode != SDL_SCANCODE_KP_ENTER && event.key.keysym.scancode != SDL_SCANCODE_ESCAPE)
					{
						key = event.key.keysym.scancode;
					}
					
					quit = SDL_TRUE;
					break;
			}
		}
	}
	
	return key;
}

unsigned getJoyStickTrigger(Sint16 *value, Uint8 *axis, int *joy_id)
{
	SDL_Event event;
	unsigned trigger = JOY_NONE;
	
	*axis = JOY_AXIS_NONE;
	*value = JOY_NONE;
	
	/*
	 * When we are finished, end the execution of the function by returning the trigger.
	 * The reason why we end the function instead of waiting for the loop to end with a boolean is that sometimes event.type can be both SDL_JOYAXISMOTION and SDL_JOYBUTTONDOWN when the user configures their controller
	 */
	while (SDL_TRUE)
	{
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_KEYDOWN:
					if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
						return trigger;
					
					break;
				case SDL_JOYBUTTONDOWN:
					*value = event.jbutton.button;
					
					*joy_id = event.jbutton.which;
					
					return trigger;
					
					case SDL_JOYAXISMOTION:
					
					// check for invalid value
					if (!(( event.jaxis.value < -32000 ) || (event.jaxis.value > 32000 )))
					{
						fprintf(stderr, "Invalid value: %d with axis: %d\n", event.jaxis.value, event.jaxis.axis);
						break;
					}
					
					// x axis
					if (event.jaxis.axis == 0)
					{
						if (event.jaxis.value > 32000)
							trigger = JOY_RIGHT;
						else if (event.jaxis.value < -32000)
							trigger = JOY_LEFT;
					}
					// y axis
					else if (event.jaxis.axis == 1)
					{
						if (event.jaxis.value > 0)
							trigger = JOY_DOWN;
						else if (event.jaxis.value < 0)
							trigger = JOY_UP;
					}
					
					*joy_id = event.jaxis.which;
					*value = event.jaxis.value;
					*axis = event.jaxis.axis;
					
					return trigger;
					break;
			}
		}
	}
	
	return trigger;
}
