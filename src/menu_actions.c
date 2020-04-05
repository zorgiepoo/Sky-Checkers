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

#include "menu_actions.h"
#include "audio.h"
#include "network.h"
#include "characters.h"
#include "input.h"
#include "platforms.h"
#include "app.h"

#include <stdlib.h>
#include <string.h>

void updateAudioMusic(ZGWindow *window, bool musicEnabled)
{
	gAudioMusicFlag = musicEnabled;
	
	if (!gAudioMusicFlag)
	{
		stopMusic();
	}
	else
	{
		bool windowFocus = ZGWindowHasFocus(window);
		playMainMenuMusic(!windowFocus);
	}
}

bool startNetworkGame(ZGWindow *window)
{
	if (gNetworkConnection != NULL && gNetworkConnection->thread != NULL)
	{
		fprintf(stderr, "game_menus: (server play) thread hasn't terminated yet.. Try again later.\n");
		return false;
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
		
		return false;
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
		
#if PLATFORM_WINDOWS
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
		
		return false;
	}
	
	freeaddrinfo(serverInfoList);
	
	retrieveLocalIPAddress(gNetworkConnection->ipAddress, sizeof(gNetworkConnection->ipAddress) - 1);
	
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
	
	initGame(window, true, false);
	
	gRedRoverInput.character = gNetworkConnection->character;
	gBlueLightningInput.character = gNetworkConnection->character;
	gGreenTreeInput.character = gNetworkConnection->character;
	
	gNetworkConnection->thread = ZGCreateThread(serverNetworkThread, "server-thread", &gNetworkConnection->numberOfPlayersToWaitFor);
	
	return true;
}

bool connectToNetworkGame(GameState *gameState)
{
	if (gNetworkConnection != NULL && gNetworkConnection->thread != NULL)
	{
		fprintf(stderr, "game_menus: (connect to server) thread hasn't terminated yet.. Try again later.\n");
		return false;
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
		
		return false;
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
		
		return false;
	}
	
	memcpy(&gNetworkConnection->hostAddress.storage, serverInfo->ai_addr, serverInfo->ai_addrlen);
	
	freeaddrinfo(serverInfoList);
	
	*gameState = GAME_STATE_CONNECTING;
	
	gNetworkConnection->thread = ZGCreateThread(clientNetworkThread, "client-thread", NULL);
	
	return true;
}

void playTutorial(ZGWindow *window)
{
	// Don't allow tutorial to play without any human or AI players
	if (gPinkBubbleGum.state == CHARACTER_AI_STATE && gRedRover.state == CHARACTER_AI_STATE && gGreenTree.state == CHARACTER_AI_STATE && gBlueLightning.state == CHARACTER_AI_STATE)
	{
		gPinkBubbleGum.backup_state = gPinkBubbleGum.state;
		gPinkBubbleGum.state = CHARACTER_HUMAN_STATE;
	}
	
	initGame(window, true, true);
}

GameState pauseGame(GameState *newGameState)
{
	pauseMusic();
	
	if (gNetworkConnection == NULL)
	{
		ZGAppSetAllowsScreenIdling(true);
	}
	
	GameState restoredGameState = *newGameState;
	*newGameState = GAME_STATE_PAUSED;
	return restoredGameState;
}

void resumeGame(GameState restoredGameState, GameState *newGameState)
{
	unPauseMusic();
	
	if (gNetworkConnection == NULL)
	{
		ZGAppSetAllowsScreenIdling(false);
	}
	
	*newGameState = restoredGameState;
}
