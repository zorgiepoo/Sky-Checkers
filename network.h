/*
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
