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

char gServerAddressString[MAX_SERVER_ADDRESS_SIZE] = "localhost";
int gServerAddressStringIndex = 9;

char gUserNameString[MAX_USER_NAME_SIZE] = "Kale";
int gUserNameStringIndex = 4;

static char *convertKeyCodeToString(unsigned theKeyCode);
static unsigned getKey(void);
static unsigned int getJoyStickTrigger(Sint16 *value, Uint8 *axis, Uint8 *hat, int *joy_id, SDL_bool *clear);

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
	initGame();
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
	
	initGame();
	
	gRedRoverInput.character = gNetworkConnection->character;
	gBlueLightningInput.character = gNetworkConnection->character;
	gGreenTreeInput.character = gNetworkConnection->character;
	
	gNetworkConnection->thread = SDL_CreateThread(serverNetworkThread, "server-thread", &gNetworkConnection->numberOfPlayersToWaitFor);
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
	
	GameState *gameState = context;
	*gameState = GAME_STATE_CONNECTING;
	
	gNetworkConnection->thread = SDL_CreateThread(clientNetworkThread, "client-thread", NULL);
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
	drawString(renderer, modelViewMatrix, preferredColor, 20.0f / 14.0f, 5.0f / 14.0f, "Configure Controllers");
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

static void drawKeyboardConfigurationInstructions(Renderer *renderer)
{
	mat4_t translationMatrix = m4_translation((vec3_t){-1.0f / 14.0f, 50.0f / 14.0f, -280.0f / 14.0f});
	
	color4_t textColor = (color4_t){0.3f, 0.2f, 1.0f, 1.0f};
	
	drawString(renderer, translationMatrix, textColor, 100.0f / 14.0f, 5.0f / 14.0f, "Enter: map controller input; Escape: cancel; Spacebar: clear.");
	
	mat4_t noticeModelViewMatrix = m4_mul(translationMatrix, m4_translation((vec3_t){0.0f / 14.0f, -20.0f / 14.0f, 0.0f / 14.0f}));
	
	drawString(renderer, noticeModelViewMatrix, textColor, 50.0f / 16.0f, 5.0f / 16.0f, "(Controllers only work in-game)");
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

static void setJoyDescription(unsigned int trigger, Uint8 axis, Uint8 hat, char *descriptionBuffer, int jsid)
{
	if (axis != JOY_AXIS_NONE)
	{
		if (trigger == JOY_UP)
		{
			sprintf(descriptionBuffer, "Joy Up");
		}
		else if (trigger == JOY_RIGHT)
		{
			sprintf(descriptionBuffer, "Joy Right");
		}
		else if (trigger == JOY_DOWN)
		{
			sprintf(descriptionBuffer, "Joy Down");
		}
		else if (trigger == JOY_LEFT)
		{
			sprintf(descriptionBuffer, "Joy Left");
		}
	}
	else if (hat != JOY_HAT_NONE)
	{
		if (trigger == JOY_UP)
		{
			sprintf(descriptionBuffer, "JoyHat Up");
		}
		else if (trigger == JOY_RIGHT)
		{
			sprintf(descriptionBuffer, "JoyHat Right");
		}
		else if (trigger == JOY_DOWN)
		{
			sprintf(descriptionBuffer, "JoyHat Down");
		}
		else if (trigger == JOY_LEFT)
		{
			sprintf(descriptionBuffer, "JoyHat Left");
		}
		else if (trigger == JOYHAT_UPRIGHT)
		{
			sprintf(descriptionBuffer, "JoyHat Right-Up");
		}
		else if (trigger == JOYHAT_DOWNRIGHT)
		{
			sprintf(descriptionBuffer, "JoyHat Right-Down");
		}
		else if (trigger == JOYHAT_DOWNLEFT)
		{
			sprintf(descriptionBuffer, "JoyHat Left-Down");
		}
		else if (trigger == JOYHAT_UPLEFT)
		{
			sprintf(descriptionBuffer, "JoyHat Left-Up");
		}
	}
	else
	{
		sprintf(descriptionBuffer, "Joy Button %d", jsid);
	}
}

void configureJoyStick(Input *input, int type)
{
	Sint16 value;
	Uint8 axis;
	Uint8 hat;
	int joy_id = -1;
	SDL_bool clear;
	
	unsigned int trigger = getJoyStickTrigger(&value, &axis, &hat, &joy_id, &clear);
	
	if (clear)
	{
		if (type == RIGHT)
		{
			input->rjs_id = JOY_NONE;
			input->rjs_axis_id = JOY_AXIS_NONE;
			input->rjs_hat_id = JOY_HAT_NONE;
			input->joy_right_id = -1;
			memset(input->joy_right_guid, 0, MAX_JOY_GUID_BUFFER_LENGTH);
			snprintf(input->joy_right, MAX_JOY_DESCRIPTION_BUFFER_LENGTH, "None");
		}
		else if (type == LEFT)
		{
			input->ljs_id = JOY_NONE;
			input->ljs_axis_id = JOY_AXIS_NONE;
			input->ljs_hat_id = JOY_HAT_NONE;
			input->joy_left_id = -1;
			memset(input->joy_left_guid, 0, MAX_JOY_GUID_BUFFER_LENGTH);
			snprintf(input->joy_left, MAX_JOY_DESCRIPTION_BUFFER_LENGTH, "None");
		}
		else if (type == DOWN)
		{
			input->djs_id = JOY_NONE;
			input->djs_axis_id = JOY_AXIS_NONE;
			input->djs_hat_id = JOY_HAT_NONE;
			input->joy_down_id = -1;
			memset(input->joy_down_guid, 0, MAX_JOY_GUID_BUFFER_LENGTH);
			snprintf(input->joy_down, MAX_JOY_DESCRIPTION_BUFFER_LENGTH, "None");
		}
		else if (type == UP)
		{
			input->ujs_id = JOY_NONE;
			input->ujs_axis_id = JOY_AXIS_NONE;
			input->ujs_hat_id = JOY_HAT_NONE;
			input->joy_up_id = -1;
			memset(input->joy_up_guid, 0, MAX_JOY_GUID_BUFFER_LENGTH);
			snprintf(input->joy_up, MAX_JOY_DESCRIPTION_BUFFER_LENGTH, "None");
		}
		else if (type == WEAPON)
		{
			input->weapjs_id = JOY_NONE;
			input->weapjs_axis_id = JOY_AXIS_NONE;
			input->weapjs_hat_id = JOY_HAT_NONE;
			input->joy_weap_id = -1;
			memset(input->joy_weap_guid, 0, MAX_JOY_GUID_BUFFER_LENGTH);
			snprintf(input->joy_weap, MAX_JOY_DESCRIPTION_BUFFER_LENGTH, "None");
		}
		
		return;
	}
	
	if (joy_id == -1)
	{
		return;
	}
	
	SDL_Joystick *joystick = SDL_JoystickFromInstanceID(joy_id);
	SDL_JoystickGUID guid = SDL_JoystickGetGUID(joystick);
	
	if (type == RIGHT)
	{
		input->joy_right_id = joy_id;
		
		if (value != (signed)JOY_NONE)
			input->rjs_id = value;
		
		input->rjs_axis_id = axis;
		input->rjs_hat_id = hat;
		
		setJoyDescription(trigger, axis, hat, input->joy_right, input->rjs_id);
		
		SDL_JoystickGetGUIDString(guid, input->joy_right_guid, MAX_JOY_GUID_BUFFER_LENGTH);
	}
	
	else if (type == LEFT)
	{
		input->joy_left_id = joy_id;
		
		if (value != (signed)JOY_NONE)
			input->ljs_id = value;
		
		input->ljs_axis_id = axis;
		input->ljs_hat_id = hat;
		
		setJoyDescription(trigger, axis, hat, input->joy_left, input->ljs_id);
		
		SDL_JoystickGetGUIDString(guid, input->joy_left_guid, MAX_JOY_GUID_BUFFER_LENGTH);
	}
	
	else if (type == UP)
	{
		input->joy_up_id = joy_id;
		input->ujs_hat_id = hat;
		
		if (value != (signed)JOY_NONE)
			input->ujs_id = value;
		
		input->ujs_axis_id = axis;
		
		setJoyDescription(trigger, axis, hat, input->joy_up, input->ujs_id);
		
		SDL_JoystickGetGUIDString(guid, input->joy_up_guid, MAX_JOY_GUID_BUFFER_LENGTH);
	}
	
	else if (type == DOWN)
	{
		input->joy_down_id = joy_id;
		input->djs_hat_id = hat;
		
		if (value != (signed)JOY_NONE)
			input->djs_id = value;
		
		input->djs_axis_id = axis;
		
		setJoyDescription(trigger, axis, hat, input->joy_down, input->djs_id);
		
		SDL_JoystickGetGUIDString(guid, input->joy_down_guid, MAX_JOY_GUID_BUFFER_LENGTH);
	}
	else if (type == WEAPON)
	{
		input->joy_weap_id = joy_id;
		
		if (value != (signed)JOY_NONE)
			input->weapjs_id = value;
		
		input->weapjs_axis_id = axis;
		input->weapjs_hat_id = hat;
		
		setJoyDescription(trigger, axis, hat, input->joy_weap, input->weapjs_id);
		
		SDL_JoystickGetGUIDString(guid, input->joy_weap_guid, MAX_JOY_GUID_BUFFER_LENGTH);
	}
}

static void drawJoyStickConfigurationInstructions(Renderer *renderer)
{
	mat4_t translationMatrix = m4_translation((vec3_t){-1.0f / 14.0f, 50.0f / 14.0f, -280.0f / 14.0f});
	
	color4_t textColor = (color4_t){0.3f, 0.2f, 1.0f, 1.0f};
	
	drawString(renderer, translationMatrix, textColor, 100.0f / 14.0f, 5.0f / 14.0f, "Enter: map controller input; Escape: cancel; Spacebar: clear.");
	
	mat4_t noticeModelViewMatrix = m4_mul(translationMatrix, m4_translation((vec3_t){0.0f / 14.0f, -20.0f / 14.0f, 0.0f / 14.0f}));
	
	drawString(renderer, noticeModelViewMatrix, textColor, 50.0f / 16.0f, 5.0f / 16.0f, "(Controllers only work in-game)");
}

void drawPinkBubbleGumRightJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	drawJoyStickConfigurationInstructions(renderer);
	
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 1.07f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Right: %s", gPinkBubbleGumInput.joy_right);
}

void pinkBubbleGumConfigRightJoyStickAction(void *context)
{
	configureJoyStick(&gPinkBubbleGumInput, RIGHT);
}

void drawPinkBubbleGumLeftJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Left: %s", gPinkBubbleGumInput.joy_left);
}

void pinkBubbleGumConfigLeftJoyStickAction(void *context)
{
	configureJoyStick(&gPinkBubbleGumInput, LEFT);
}

void drawPinkBubbleGumUpJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -1.07f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Up: %s", gPinkBubbleGumInput.joy_up);
}

void pinkBubbleGumConfigUpJoyStickAction(void *context)
{
	configureJoyStick(&gPinkBubbleGumInput, UP);
}

void drawPinkBubbleGumDownJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -2.14f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Down: %s", gPinkBubbleGumInput.joy_down);
}

void pinkBubbleGumConfigDownJoyStickAction(void *context)
{
	configureJoyStick(&gPinkBubbleGumInput, DOWN);
}

void drawPinkBubbleGumFireJoyStickConfig(Renderer *renderer, color4_t preferredColor)
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

void drawRedRoverRightJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	drawJoyStickConfigurationInstructions(renderer);
	
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 1.07f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Right: %s", gRedRoverInput.joy_right);
}

void redRoverConfigRightJoyStickAction(void *context)
{
	configureJoyStick(&gRedRoverInput, RIGHT);
}

void drawRedRoverLeftJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Left: %s", gRedRoverInput.joy_left);
}

void redRoverConfigLeftJoyStickAction(void *context)
{
	configureJoyStick(&gRedRoverInput, LEFT);
}

void drawRedRoverUpJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -1.07f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Up: %s", gRedRoverInput.joy_up);
}

void redRoverConfigUpJoyStickAction(void *context)
{
	configureJoyStick(&gRedRoverInput, UP);
}

void drawRedRoverDownJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -2.14f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Down: %s", gRedRoverInput.joy_down);
}

void redRoverConfigDownJoyStickAction(void *context)
{
	configureJoyStick(&gRedRoverInput, DOWN);
}

void drawRedRoverFireJoyStickConfig(Renderer *renderer, color4_t preferredColor)
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

void drawGreenTreeRightJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	drawJoyStickConfigurationInstructions(renderer);
	
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 1.07f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Right: %s", gGreenTreeInput.joy_right);
}

void greenTreeConfigRightJoyStickAction(void *context)
{
	configureJoyStick(&gGreenTreeInput, RIGHT);
}

void drawGreenTreeLeftJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Left: %s", gGreenTreeInput.joy_left);
}

void greenTreeConfigLeftJoyStickAction(void *context)
{
	configureJoyStick(&gGreenTreeInput, LEFT);
}

void drawGreenTreeUpJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -1.07f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Up: %s", gGreenTreeInput.joy_up);
}

void greenTreeConfigUpJoyStickAction(void *context)
{
	configureJoyStick(&gGreenTreeInput, UP);
}

void drawGreenTreeDownJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -2.14f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Down: %s", gGreenTreeInput.joy_down);
}

void greenTreeConfigDownJoyStickAction(void *context)
{
	configureJoyStick(&gGreenTreeInput, DOWN);
}

void drawGreenTreeFireJoyStickConfig(Renderer *renderer, color4_t preferredColor)
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

void drawBlueLightningRightJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	drawJoyStickConfigurationInstructions(renderer);
	
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 1.07f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Right: %s", gBlueLightningInput.joy_right);
}

void blueLightningConfigRightJoyStickAction(void *context)
{
	configureJoyStick(&gBlueLightningInput, RIGHT);
}

void drawBlueLightningLeftJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, 0.00f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Left: %s", gBlueLightningInput.joy_left);
}

void blueLightningConfigLeftJoyStickAction(void *context)
{
	configureJoyStick(&gBlueLightningInput, LEFT);
}

void drawBlueLightningUpJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -1.07f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Up: %s", gBlueLightningInput.joy_up);
}

void blueLightningConfigUpJoyStickAction(void *context)
{
	configureJoyStick(&gBlueLightningInput, UP);
}

void drawBlueLightningDownJoyStickConfig(Renderer *renderer, color4_t preferredColor)
{
	mat4_t modelViewMatrix = m4_translation((vec3_t){-0.07f, -2.14f, -20.00f});	
	drawStringf(renderer, modelViewMatrix, preferredColor, 15.0f / 14.0f, 5.0f / 14.0f, "Down: %s", gBlueLightningInput.joy_down);
}

void blueLightningConfigDownJoyStickAction(void *context)
{
	configureJoyStick(&gBlueLightningInput, DOWN);
}

void drawBlueLightningFireJoyStickConfig(Renderer *renderer, color4_t preferredColor)
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
	// Four characters that each have their own menu + five configured menu actions (right, up, left, down, fire)
	Menu *characterConfigureKeys = 				malloc(sizeof(Menu) * 4 * 6);
	Menu *joyStickConfig =						malloc(sizeof(Menu) * 4 * 6);
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
	
	// Joystick menus
	
	// PinkBubbleGum configs
	joyStickConfig[0].draw = drawPinkBubbleGumConfigJoyStick;
	joyStickConfig[0].action = pinkBubbleGumConfigJoyStickAction;
	
	joyStickConfig[1].draw = drawPinkBubbleGumRightJoyStickConfig;
	joyStickConfig[1].action = pinkBubbleGumConfigRightJoyStickAction;
	
	joyStickConfig[2].draw = drawPinkBubbleGumLeftJoyStickConfig;
	joyStickConfig[2].action = pinkBubbleGumConfigLeftJoyStickAction;
	
	joyStickConfig[3].draw = drawPinkBubbleGumUpJoyStickConfig;
	joyStickConfig[3].action = pinkBubbleGumConfigUpJoyStickAction;
	
	joyStickConfig[4].draw = drawPinkBubbleGumDownJoyStickConfig;
	joyStickConfig[4].action = pinkBubbleGumConfigDownJoyStickAction;
	
	joyStickConfig[5].draw = drawPinkBubbleGumFireJoyStickConfig;
	joyStickConfig[5].action = pinkBubbleGumConfigFireJoyStickAction;
	
	// RedRover configs
	joyStickConfig[6].draw = drawRedRoverConfigJoyStick;
	joyStickConfig[6].action = redRoverConfigJoyStickAction;
	
	joyStickConfig[7].draw = drawRedRoverRightJoyStickConfig;
	joyStickConfig[7].action = redRoverConfigRightJoyStickAction;
	
	joyStickConfig[8].draw = drawRedRoverLeftJoyStickConfig;
	joyStickConfig[8].action = redRoverConfigLeftJoyStickAction;
	
	joyStickConfig[9].draw = drawRedRoverUpJoyStickConfig;
	joyStickConfig[9].action = redRoverConfigUpJoyStickAction;
	
	joyStickConfig[10].draw = drawRedRoverDownJoyStickConfig;
	joyStickConfig[10].action = redRoverConfigDownJoyStickAction;
	
	joyStickConfig[11].draw = drawRedRoverFireJoyStickConfig;
	joyStickConfig[11].action = redRoverConfigFireJoyStickAction;
	
	// GreenTree configs
	joyStickConfig[12].draw = drawGreenTreeConfigJoyStick;
	joyStickConfig[12].action = greenTreeConfigJoyStickAction;
	
	joyStickConfig[13].draw = drawGreenTreeRightJoyStickConfig;
	joyStickConfig[13].action = greenTreeConfigRightJoyStickAction;
	
	joyStickConfig[14].draw = drawGreenTreeLeftJoyStickConfig;
	joyStickConfig[14].action = greenTreeConfigLeftJoyStickAction;
	
	joyStickConfig[15].draw = drawGreenTreeUpJoyStickConfig;
	joyStickConfig[15].action = greenTreeConfigUpJoyStickAction;
	
	joyStickConfig[16].draw = drawGreenTreeDownJoyStickConfig;
	joyStickConfig[16].action = greenTreeConfigDownJoyStickAction;
	
	joyStickConfig[17].draw = drawGreenTreeFireJoyStickConfig;
	joyStickConfig[17].action = greenTreeConfigFireJoyStickAction;
	
	// BlueLightning configs
	joyStickConfig[18].draw = drawBlueLightningConfigJoyStick;
	joyStickConfig[18].action = blueLightningConfigJoyStickAction;
	
	joyStickConfig[19].draw = drawBlueLightningRightJoyStickConfig;
	joyStickConfig[19].action = blueLightningConfigRightJoyStickAction;
	
	joyStickConfig[20].draw = drawBlueLightningLeftJoyStickConfig;
	joyStickConfig[20].action = blueLightningConfigLeftJoyStickAction;
	
	joyStickConfig[21].draw = drawBlueLightningUpJoyStickConfig;
	joyStickConfig[21].action = blueLightningConfigUpJoyStickAction;
	
	joyStickConfig[22].draw = drawBlueLightningDownJoyStickConfig;
	joyStickConfig[22].action = blueLightningConfigDownJoyStickAction;
	
	joyStickConfig[23].draw = drawBlueLightningFireJoyStickConfig;
	joyStickConfig[23].action = blueLightningConfigFireJoyStickAction;
	
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
	
	// Configure keys and joy stick submenus
	for (int characterIndex = 0; characterIndex < 4; characterIndex++)
	{
		addSubMenu(configureKeysMenu, &characterConfigureKeys[characterIndex * 6]);
		addSubMenu(configureJoySticksMenu, &joyStickConfig[characterIndex * 6]);
		
		for (int submenuIndex = 1; submenuIndex < 6; submenuIndex++)
		{
			addSubMenu(&characterConfigureKeys[characterIndex * 6], &characterConfigureKeys[characterIndex * 6 + submenuIndex]);
			addSubMenu(&joyStickConfig[characterIndex * 6], &joyStickConfig[characterIndex * 6 + submenuIndex]);
		}
	}
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

unsigned int getJoyStickTrigger(Sint16 *value, Uint8 *axis, Uint8 *hat, int *joy_id, SDL_bool *clear)
{
	SDL_Event event;
	unsigned int trigger = JOY_NONE;
	
	*axis = JOY_AXIS_NONE;
	*value = JOY_NONE;
	*hat = JOY_HAT_NONE;
	*clear = SDL_FALSE;
	
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
				
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE)
				{
					*clear = SDL_TRUE;
					return trigger;
				}
				
				break;
			case SDL_JOYBUTTONDOWN:
				*value = event.jbutton.button;
				
				*joy_id = event.jbutton.which;
				
				return trigger;
				
			case SDL_JOYAXISMOTION:
				// check for invalid value
				if (!(( event.jaxis.value < -VALID_ANALOG_MAGNITUDE ) || (event.jaxis.value > VALID_ANALOG_MAGNITUDE )))
				{
					fprintf(stderr, "Invalid value: %d with axis: %d\n", event.jaxis.value, event.jaxis.axis);
					break;
				}
				
				// x axis
				if (event.jaxis.axis == 0)
				{
					if (event.jaxis.value > VALID_ANALOG_MAGNITUDE)
						trigger = JOY_RIGHT;
					else if (event.jaxis.value < -VALID_ANALOG_MAGNITUDE)
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
			case SDL_JOYHATMOTION:
				if (event.jhat.value == 0)
				{
					break;
				}
				else
				{
					*joy_id = event.jhat.which;
					*value = event.jhat.value;
					*hat = event.jhat.hat;
					
					if (event.jhat.value == SDL_HAT_LEFTUP)
					{
						trigger = JOYHAT_UPLEFT;
					}
					else if (event.jhat.value == SDL_HAT_LEFTDOWN)
					{
						trigger = JOYHAT_DOWNLEFT;
					}
					else if (event.jhat.value == SDL_HAT_RIGHTUP)
					{
						trigger = JOYHAT_UPRIGHT;
					}
					else if (event.jhat.value == SDL_HAT_RIGHTDOWN)
					{
						trigger = JOYHAT_DOWNRIGHT;
					}
					else if (event.jhat.value == SDL_HAT_RIGHT)
					{
						trigger = JOY_RIGHT;
					}
					else if (event.jhat.value == SDL_HAT_LEFT)
					{
						trigger = JOY_LEFT;
					}
					else if (event.jhat.value == SDL_HAT_UP)
					{
						trigger = JOY_UP;
					}
					else if (event.jhat.value == SDL_HAT_DOWN)
					{
						trigger = JOY_DOWN;
					}
					return trigger;
				}
			}
		}
	}
	
	return trigger;
}
