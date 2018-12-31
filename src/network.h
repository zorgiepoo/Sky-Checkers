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
#include "characters.h"

#define NETWORK_SERVER_TYPE 1
#define NETWORK_CLIENT_TYPE 2

extern const Uint16 NETWORK_PORT;

typedef struct
{
	float x, y, z;
	int direction;
	int pointing_direction;
	uint32_t ticks;
} CharacterMovement;

#define CHARACTER_MOVEMENTS_CAPACITY 20

typedef struct
{
	// Only writable before threads are created
	int type;
	int socket;
	
	// Writable & readable from main thread only
	Character *character;
	uint32_t numberOfPlayersToWaitFor;
	
	// Writable before thread is created or during creation
	// Only used from main thread
	SDL_Thread *thread;
	
	// Below is the stuff only applicable to client
	
	// Only writable before client thread is created
	struct sockaddr_in hostAddress;
	// Writable & readable from main thread only
	int characterLives;
	
	// Keeping track of client half-ping
	// Only readable/writable from main thread
	uint32_t lastMovementMessageTime;
	uint32_t averageIncomingMovementMessageTime;
	uint32_t incomingMovementMessageTimes[10];
	uint32_t incomingMovementMessageTimeIndex;
	
	// Keeping track of past character movements
	// Only used by client currently and only readable/writable from main thread
	CharacterMovement characterMovements[4][CHARACTER_MOVEMENTS_CAPACITY];
	uint32_t characterMovementCounts[4];
} NetworkConnection;

typedef enum
{
	QUIT_MESSAGE_TYPE = 0,
	MOVEMENT_REQUEST_MESSAGE_TYPE = 1,
	CHARACTER_FIRED_REQUEST_MESSAGE_TYPE = 2,
	NUMBER_OF_PLAYERS_WAITING_FOR_MESSAGE_TYPE = 3,
	NET_NAME_MESSAGE_TYPE = 4,
	START_GAME_MESSAGE_TYPE = 5,
	GAME_START_NUMBER_UPDATE_MESSAGE_TYPE = 6,
	CHARACTER_DIED_UPDATE_MESSAGE_TYPE = 7,
	CHARACTER_MOVED_UPDATE_MESSAGE_TYPE = 8,
	CHARACTER_KILLED_UPDATE_MESSAGE_TYPE = 9,
	GAME_RESET_MESSAGE_TYPE = 10,
	FIRST_SERVER_RESPONSE_MESSAGE_TYPE = 11,
	FIRST_CLIENT_RESPONSE_MESSAGE_TYPE = 12,
	FIRST_DATA_TO_CLIENT_MESSAGE_TYPE = 13,
	WELCOME_MESSAGE_TO_SERVER_MESSAGE_TYPE = 14,
	CHARACTER_FIRED_UPDATE_MESSAGE_TYPE = 15,
	ACK_MESSAGE_TYPE = 16,
	COLOR_TILE_MESSAGE_TYPE = 17,
	TILE_FALLING_DOWN_MESSAGE_TYPE = 18,
	RECOVER_TILE_MESSAGE_TYPE = 19
} MessageType;

typedef struct
{
	int direction;
} CharacterMovementRequest;

typedef struct
{
	int characterID;
} CharacterFiredRequest;

typedef struct
{
	int characterID;
	float x, y;
	int direction;
} CharacterFiredUpdate;

typedef struct
{
	int characterID;
	char *netName;
} CharacterNetNameRequest;

typedef struct
{
	int characterID;
	int characterLives;
} CharacterDiedUpdate;

typedef struct
{
	int characterID;
	float x, y, z;
	int direction;
	int pointing_direction;
	uint32_t timestamp;
} CharacterMovedUpdate;

typedef struct
{
	int characterID;
	int kills;
} CharacterKilledUpdate;

typedef struct
{
	int characterID;
	float x, y;
} CharacterSpawnedUpdate;

typedef struct
{
	int slotID;
	int characterLives;
} FirstServerResponse;

typedef struct
{
	struct sockaddr_in clientAddress;
	char *netName;
	int slotID;
	int numberOfPlayersToWaitFor;
} FirstClientResponse;

typedef struct
{
	char *netNames[4];
	int characterID;
	int numberOfPlayersToWaitFor;
} FirstDataToClient;

typedef struct
{
	char *netName;
} WelcomeMessage;

typedef struct
{
	int characterID;
	int tileIndex;
} ColorTileMessage;

typedef struct
{
	int tileIndex;
	SDL_bool dead;
} FallingTileMessage;

typedef struct
{
	int tileIndex;
} RecoverTileMessage;

typedef struct
{
	MessageType type;
	uint64_t packetNumber;
	int addressIndex;
	union
	{
		CharacterMovementRequest movementRequest;
		CharacterFiredRequest firedRequest;
		CharacterFiredUpdate firedUpdate;
		CharacterNetNameRequest netNameRequest;
		CharacterDiedUpdate diedUpdate;
		CharacterMovedUpdate movedUpdate;
		CharacterKilledUpdate killedUpdate;
		CharacterSpawnedUpdate spawnedUpdate;
		FirstServerResponse firstServerResponse;
		FirstClientResponse firstClientResponse;
		FirstDataToClient firstDataToClient;
		WelcomeMessage weclomeMessage;
		ColorTileMessage colorTile;
		FallingTileMessage fallingTile;
		RecoverTileMessage recoverTile;
		
		uint32_t numberOfWaitingPlayers;
		int32_t gameStartNumber;
	};
} GameMessage;

typedef struct
{
	GameMessage *messages;
	uint32_t count;
	uint32_t capacity;
	SDL_mutex *mutex;
} GameMessageArray;

// For the server.. This should be initialized to zero before creating the server thread.
int gCurrentSlot;

GameMessageArray gGameMessagesFromNet;
GameMessageArray gGameMessagesToNet;

extern NetworkConnection *gNetworkConnection;

void initializeNetworkBuffers(void);
GameMessage *popNetworkMessages(GameMessageArray *messageArray, uint32_t *count);

void networkInitialization(void);
void cleanupStateFromNetwork(void);

void syncNetworkState(SDL_Window *window, float timeDelta);

int serverNetworkThread(void *unused);
int clientNetworkThread(void *context);

void sendToClients(int exception, GameMessage *message);

void sendToServer(GameMessage message);

void closeSocket(int sockfd);
