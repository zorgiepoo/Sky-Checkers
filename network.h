/*
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
 
 * Coder: Mayur Pawashe (Zorg)
 * http://zorg.tejat.net
 */

#pragma once

#include "maincore.h"
#include "input.h"

extern const int NETWORK_SERVER_TYPE;
extern const int NETWORK_CLIENT_TYPE;
extern const Uint16 NETWORK_PORT;

typedef struct
{
	Input *input;
	int type;
	int socket;
	
	int numberOfPlayersToWaitFor;
	SDL_bool shouldRun;
	
	// below is the stuff only applicable to client
	struct sockaddr_in hostAddress;
	int characterLives;
	SDL_Thread *thread;
	SDL_bool isConnected;
} NetworkConnection;

extern NetworkConnection *gNetworkConnection;

void networkInitialization(void);
void networkCleanup(void);

int serverNetworkThread(void *unused);
int clientNetworkThread(void *unused);

void sendToClients(int exception, const char *format, ...);

void closeSocket(int sockfd);
