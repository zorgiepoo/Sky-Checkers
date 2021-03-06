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

#include "platforms.h"
#include "characters.h"
#include "thread.h"
#include "window.h"
#include "globals.h"

#if PLATFORM_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#endif

#define NETWORK_SERVER_TYPE 1
#define NETWORK_CLIENT_TYPE 2

#define NETWORK_PORT "4893"

#if PLATFORM_WINDOWS
typedef SOCKET socket_t;
typedef int socket_size_t;
#else
typedef int socket_t;
typedef size_t socket_size_t;
#endif

typedef struct
{
	float x, y;
	int32_t direction;
	int32_t pointing_direction;
	uint32_t ticks;
	uint8_t dead;
} CharacterMovement;

#define CHARACTER_MOVEMENTS_CAPACITY 20

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
	RECOVER_TILE_MESSAGE_TYPE = 19,
	LAGGED_OUT_MESSAGE_TYPE = 20,
	PING_MESSAGE_TYPE = 21,
	PONG_MESSAGE_TYPE = 22
} MessageType;

typedef struct
{
	uint8_t direction;
} CharacterMovementRequest;

typedef struct
{
	uint8_t characterID;
} CharacterFiredRequest;

typedef struct
{
	float x, y;
	uint8_t characterID;
	int8_t direction;
} CharacterFiredUpdate;

typedef struct
{
	uint8_t characterID;
	char *netName;
} CharacterNetNameRequest;

typedef struct
{
	uint8_t characterID;
	uint8_t characterLives;
} CharacterDiedUpdate;

typedef struct
{
	float x, y;
	uint8_t characterID;
	uint8_t direction;
	uint8_t pointing_direction;
	uint8_t dead;
} CharacterMovedUpdate;

typedef struct
{
	uint8_t characterID;
	uint8_t kills;
} CharacterKilledUpdate;

typedef struct
{
	int32_t characterID;
	float x, y;
} CharacterSpawnedUpdate;

typedef struct
{
	uint8_t slotID;
	uint8_t characterLives;
} FirstServerResponse;

typedef struct
{
	char *netName;
	int32_t slotID;
	uint8_t numberOfPlayersToWaitFor;
} FirstClientResponse;

typedef struct
{
	char *netNames[4];
	uint8_t characterID;
	uint8_t numberOfPlayersToWaitFor;
} FirstDataToClient;

typedef struct
{
	char *netName;
	uint8_t version;
} WelcomeMessage;

typedef struct
{
	uint8_t characterID;
	uint8_t tileIndex;
} ColorTileMessage;

typedef struct
{
	int8_t dead;
	uint8_t tileIndex;
} FallingTileMessage;

typedef struct
{
	uint8_t tileIndex;
} RecoverTileMessage;

typedef struct
{
	uint8_t characterID;
} LaggedOutMessage;

typedef struct
{
	MessageType type;
	uint32_t packetNumber;
	int addressIndex;
	uint32_t ticks;
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
		WelcomeMessage welcomeMessage;
		ColorTileMessage colorTile;
		FallingTileMessage fallingTile;
		RecoverTileMessage recoverTile;
		LaggedOutMessage laggedUpdate;
		uint32_t pingTimestamp;
		uint32_t pongTimestamp;
		
		uint8_t numberOfWaitingPlayers;
		uint8_t gameStartNumber;
	};
} GameMessage;

typedef struct
{
	GameMessage *messages;
	uint32_t count;
	uint32_t capacity;
	ZGMutex *mutex;
} GameMessageArray;

// Use a union to avoid violating strict aliasing
// instead of casting to sockaddr_storage
typedef union
{
	struct sockaddr sa;
	struct sockaddr_in sa_in;
	struct sockaddr_in6 sa_in6;
	struct sockaddr_storage storage;
} SocketAddress;

typedef struct
{
	// Only writable before threads are created
	int type;
	socket_t socket;
	
	// Writable & readable from main thread only
	Character *character;
	uint8_t numberOfPlayersToWaitFor;
	
	// Writable before thread is created or during creation
	// Only used from main thread
	ZGThread thread;
	
	union
	{
		// Client state
		struct
		{
			// Only writable before client thread is created
			SocketAddress hostAddress;
			// Writable & readable from main thread only
			uint8_t characterLives;
			
			// Keeping track of half-ping from the server
			// Only readable/writable from main thread
			uint32_t serverHalfPing;
			uint32_t recentServerHalfPings[10];
			uint32_t recentServerHalfPingIndex;
			
			// Keeping track of past character movements
			// Only used by client currently and only readable/writable from main thread
			CharacterMovement characterMovements[4][CHARACTER_MOVEMENTS_CAPACITY];
			uint32_t characterMovementCounts[4];
			
			// Keeping track of past character trigger messages, only readable/writable from main thread
			GameMessage *characterTriggerMessages;
			uint32_t characterTriggerMessagesCount;
			uint32_t characterTriggerMessagesCapacity;
		};
		
		// Server state
		struct
		{
			// Only readable/writable from server thread
			SocketAddress clientAddresses[3];
			// Keeping track of half-ping from clients
			// Only readable/writable from main thread
			uint32_t clientHalfPings[3];
			uint32_t recentClientHalfPings[3][10];
			uint32_t recentClientHalfPingIndices[3];
			// Local IP address
			// Only readable/writable from main thread
			char ipAddress[MAX_SERVER_ADDRESS_SIZE];
		};
	};
} NetworkConnection;

// For the server.. These should be initialized to zero before creating the server thread.
extern uint8_t gCurrentSlot;
extern uint8_t gClientStates[3];

extern NetworkConnection *gNetworkConnection;

void initializeNetworkBuffers(void);
GameMessage *popNetworkMessages(GameMessageArray *messageArray, uint32_t *count);

void initializeNetwork(void);
void deinitializeNetwork(void);

void syncNetworkState(ZGWindow *window, float timeDelta, GameState gameState);

void setPredictedDirection(Character *character, int direction);

int serverNetworkThread(void *unused);
int clientNetworkThread(void *context);

void sendToClients(int exception, GameMessage *message);

void sendToServer(GameMessage message);

void closeSocket(socket_t sockfd);

// Enumerates through host names and returns the first enumerated en0 one.
// An IPv4 address will take priority over an IPv6 one due to being more user friendly
void retrieveLocalIPAddress(char *ipAddressBuffer, size_t bufferSize);
