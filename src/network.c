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

#include "network.h"
#include "animation.h"
#include "game_menus.h"
#include "utilities.h"

const Uint16 NETWORK_PORT =				4893;

#define MAX_NETWORK_BUFFER	256

NetworkConnection *gNetworkConnection = NULL;

struct sockaddr_in gClientAddress[3];

static SDL_mutex *gCurrentSlotMutex;

static void pushNetworkMessage(GameMessageArray *messageArray, GameMessage message);
static void depleteNetworkMessages(GameMessageArray *messageArray);

void syncNetworkState(void)
{
	if (gNetworkConnection == NULL)
	{
		return;
	}
	
	uint32_t messagesCount = 0;
	GameMessage *messagesToRead = popNetworkMessages(&gGameMessagesFromNet, &messagesCount);
	if (messagesToRead != NULL)
	{
		for (uint32_t messageIndex = 0; messageIndex < messagesCount && (gNetworkConnection != NULL); messageIndex++)
		{
			GameMessage message = messagesToRead[messageIndex];
			switch (message.type)
			{
				case WELCOME_MESSAGE_TO_SERVER_MESSAGE_TYPE:
					break;
				case QUIT_MESSAGE_TYPE:
					endGame();
					cleanupStateFromNetwork();
					
					closeSocket(gNetworkConnection->socket);
#ifdef WINDOWS
					WSACleanup();
#endif
					
					SDL_Thread *thread = gNetworkConnection->thread;
					
					free(gNetworkConnection);
					gNetworkConnection = NULL;
					
					depleteNetworkMessages(&gGameMessagesFromNet);
					depleteNetworkMessages(&gGameMessagesToNet);
					
					if (thread != NULL)
					{
						// We need to wait for the thread to exit, otherwise we'll have a resource leak
						SDL_WaitThread(thread, NULL);
					}
					
					break;
				case MOVEMENT_REQUEST_MESSAGE_TYPE:
				{
					int direction = message.movementRequest.direction;
					int characterID = message.addressIndex + 1;
					
					Character *character = getCharacter(characterID);
					character->direction = direction;
					turnCharacter(character, direction);
					
					break;
				}
				case CHARACTER_FIRED_MESSAGE_TYPE:
				{
					int characterID = message.firingRequest.characterID;
					Character *character = getCharacter(characterID);
					character->weap->fired = SDL_TRUE;
					
					break;
				}
				case NUMBER_OF_PLAYERS_WAITING_FOR_MESSAGE_TYPE:
					gNetworkConnection->numberOfPlayersToWaitFor = message.numberOfWaitingPlayers;
					break;
				case NET_NAME_MESSAGE_TYPE:
				{
					Character *character = getCharacter(message.netNameRequest.characterID);
					character->netName = message.netNameRequest.netName;
					break;
				}
				case START_GAME_MESSAGE_TYPE:
					gPinkBubbleGum.netState = NETWORK_PLAYING_STATE;
					break;
				case GAME_START_NUMBER_UPDATE_MESSAGE_TYPE:
					gGameStartNumber = message.gameStartNumber;
					break;
				case CHARACTER_DIED_UPDATE_MESSAGE_TYPE:
				{
					Character *character = getCharacter(message.diedUpdate.characterID);
					character->lives = message.diedUpdate.characterLives;
					
					prepareCharactersDeath(character);
					decideWhetherToMakeAPlayerAWinner(character);
					break;
				}
				case CHARACTER_MOVED_UPDATE_MESSAGE_TYPE:
				{
					Character *character = getCharacter(message.movedUpdate.characterID);
					character->x = message.movedUpdate.x;
					character->y = message.movedUpdate.y;
					character->z = message.movedUpdate.z;
					character->direction = message.movedUpdate.direction;
					turnCharacter(character, character->direction);
					break;
				}
				case CHARACTER_KILLED_UPDATE_MESSAGE_TYPE:
				{
					Character *character = getCharacter(message.killedUpdate.characterID);
					character->kills = message.killedUpdate.kills;
					break;
				}
				case CHARACTER_SPAWNED_UPDATE_MESSAGE_TYPE:
				{
					Character *character = getCharacter(message.spawnedUpdate.characterID);
					character->x = message.spawnedUpdate.x;
					character->y = message.spawnedUpdate.y;
					character->z = 2.0f;
					character->active = SDL_TRUE;
					break;
				}
				case GAME_RESET_MESSAGE_TYPE:
					gGameShouldReset = SDL_TRUE;
					break;
				case FIRST_SERVER_RESPONSE_MESSAGE_TYPE:
				{
					int slotID = message.firstServerResponse.slotID;
					float x = message.firstServerResponse.x;
					float y = message.firstServerResponse.y;
					int direction = message.firstServerResponse.direction;
					int characterLives = message.firstServerResponse.characterLives;
					
					gNetworkConnection->characterLives = characterLives;
					
					if (slotID + 1 == RED_ROVER)
					{
						gNetworkConnection->character = &gRedRover;
					}
					else if (slotID + 1 == GREEN_TREE)
					{
						gNetworkConnection->character = &gGreenTree;
					}
					else if (slotID + 1 == BLUE_LIGHTNING)
					{
						gNetworkConnection->character = &gBlueLightning;
					}
					
					gNetworkConnection->character->netName = gUserNameString;
					
					gPinkBubbleGumInput.character = gNetworkConnection->character;
					gRedRoverInput.character = gNetworkConnection->character;
					gBlueLightningInput.character = gNetworkConnection->character;
					gGreenTreeInput.character = gNetworkConnection->character;
					
					// server is pending
					gPinkBubbleGum.netState = NETWORK_PENDING_STATE;
					
					initGame();
					
					gPinkBubbleGum.lives = gNetworkConnection->characterLives;
					gRedRover.lives = gNetworkConnection->characterLives;
					gGreenTree.lives = gNetworkConnection->characterLives;
					gBlueLightning.lives = gNetworkConnection->characterLives;
					
					gNetworkConnection->character->x = x;
					gNetworkConnection->character->y = y;
					gNetworkConnection->character->y = 2.0f;
					gNetworkConnection->character->direction = direction;
					turnCharacter(gNetworkConnection->character, direction);
					
					break;
				}
				case FIRST_CLIENT_RESPONSE_MESSAGE_TYPE:
				{
					Character *character = getCharacter(message.firstClientResponse.slotID);
					character->netName = message.firstClientResponse.netName;
					character->netState = NETWORK_PLAYING_STATE;
					
					GameMessage messageBack;
					messageBack.type = FIRST_DATA_TO_CLIENT_MESSAGE_TYPE;
					messageBack.addressIndex = message.addressIndex;
					messageBack.firstDataToClient.characterID = message.firstClientResponse.slotID;
					messageBack.firstDataToClient.numberOfPlayersToWaitFor = message.firstClientResponse.numberOfPlayersToWaitFor;
					
					for (int characterIndex = RED_ROVER; characterIndex <= PINK_BUBBLE_GUM; characterIndex++)
					{
						CharacterInfo *characterInfo = &messageBack.firstDataToClient.characterInfo[characterIndex - 1];
						
						Character *character = getCharacter(characterIndex);
						
						characterInfo->x = character->x;
						characterInfo->y = character->y;
						characterInfo->z = character->z;
						characterInfo->direction = character->direction;
						characterInfo->netName = character->netName;
					}
					
					pushNetworkMessage(&gGameMessagesToNet, messageBack);
					
					break;
				}
				case FIRST_DATA_TO_CLIENT_MESSAGE_TYPE:
					break;
			}
		}
		free(messagesToRead);
	}
}

static void sendData(int socket, void *data, size_t size, struct sockaddr_in *address)
{
	sendto(socket, data, size, 0, (struct sockaddr *)address, sizeof(*address));
}

static int characterIDForClientAddress(struct sockaddr_in *address)
{
	for (int clientIndex = 0; clientIndex < gCurrentSlot; clientIndex++)
	{
		if (memcmp(address, &gClientAddress[clientIndex], sizeof(*address)) == 0)
		{
			return clientIndex + 1;
		}
	}
	return NO_CHARACTER;
}

int serverNetworkThread(void *initialNumberOfPlayersToWaitForPtr)
{
	gCurrentSlot = 0;
	int numberOfPlayersToWaitFor = *(int *)initialNumberOfPlayersToWaitForPtr;
	
	while (SDL_TRUE)
	{
		uint32_t messagesCount = 0;
		GameMessage *messagesAvailable = popNetworkMessages(&gGameMessagesToNet, &messagesCount);
		if (messagesAvailable != NULL)
		{
			SDL_bool needsToQuit = SDL_FALSE;
			
			for (uint32_t messageIndex = 0; messageIndex < messagesCount; messageIndex++)
			{
				GameMessage message = messagesAvailable[messageIndex];
				struct sockaddr_in *address = message.addressIndex < 0 ? NULL :  &gClientAddress[message.addressIndex];
				
				switch (message.type)
				{
					case WELCOME_MESSAGE_TO_SERVER_MESSAGE_TYPE:
						break;
					case QUIT_MESSAGE_TYPE:
					{
						if (address != NULL)
						{
							char buffer[] = "qu";
							sendData(gNetworkConnection->socket, buffer, sizeof(buffer) - 1, address);
						}
						
						needsToQuit = SDL_TRUE;
						
						break;
					}
					case MOVEMENT_REQUEST_MESSAGE_TYPE:
					{
						char buffer[256];
						snprintf(buffer, sizeof(buffer) - 1, "rm%d", message.movementRequest.direction);
						
						sendData(gNetworkConnection->socket, buffer, sizeof(buffer) - 1, address);
						
						break;
					}
					case CHARACTER_FIRED_MESSAGE_TYPE:
					{
						char buffer[256];
						snprintf(buffer, sizeof(buffer) - 1, "sw%d", message.firingRequest.characterID);
						
						sendData(gNetworkConnection->socket, buffer, strlen(buffer), address);
						
						break;
					}
					case NUMBER_OF_PLAYERS_WAITING_FOR_MESSAGE_TYPE:
					{
						char buffer[256];
						snprintf(buffer, sizeof(buffer) - 1, "nw%d", message.numberOfWaitingPlayers);
						
						sendData(gNetworkConnection->socket, buffer, strlen(buffer), address);
						
						break;
					}
					case NET_NAME_MESSAGE_TYPE:
					{
						char buffer[256];
						snprintf(buffer, sizeof(buffer) - 1, "nn%d %s", message.netNameRequest.characterID, message.netNameRequest.netName);
						
						sendData(gNetworkConnection->socket, buffer, strlen(buffer), address);
						
						break;
					}
					case START_GAME_MESSAGE_TYPE:
					{
						char buffer[] = "sg";
						sendData(gNetworkConnection->socket, buffer, sizeof(buffer) - 1, address);
						
						break;
					}
					case GAME_START_NUMBER_UPDATE_MESSAGE_TYPE:
					{
						char buffer[256];
						snprintf(buffer, sizeof(buffer) - 1, "gs%d", message.gameStartNumber);
						
						sendData(gNetworkConnection->socket, buffer, strlen(buffer), address);
						
						break;
					}
					case CHARACTER_DIED_UPDATE_MESSAGE_TYPE:
					{
						char buffer[256];
						snprintf(buffer, sizeof(buffer) - 1, "pk%d %d", message.diedUpdate.characterID, message.diedUpdate.characterLives);
						
						sendData(gNetworkConnection->socket, buffer, strlen(buffer), address);
						
						break;
					}
					case CHARACTER_MOVED_UPDATE_MESSAGE_TYPE:
					{
						char buffer[256];
						snprintf(buffer, sizeof(buffer) - 1, "mo%d %f %f %f %d", message.movedUpdate.characterID, message.movedUpdate.x, message.movedUpdate.y, message.movedUpdate.z, message.movedUpdate.direction);
						
						sendData(gNetworkConnection->socket, buffer, strlen(buffer), address);
						
						break;
					}
					case CHARACTER_KILLED_UPDATE_MESSAGE_TYPE:
					{
						char buffer[256];
						snprintf(buffer, sizeof(buffer) - 1, "ck%d %d", IDOfCharacter(message.killedUpdate.characterID), message.killedUpdate.kills);
						
						sendData(gNetworkConnection->socket, buffer, strlen(buffer), address);
						
						break;
					}
					case CHARACTER_SPAWNED_UPDATE_MESSAGE_TYPE:
					{
						char buffer[256];
						snprintf(buffer, sizeof(buffer) - 1, "sp%d %f %f", message.spawnedUpdate.characterID, message.spawnedUpdate.x, message.spawnedUpdate.y);
						
						sendData(gNetworkConnection->socket, buffer, strlen(buffer), address);
						
						break;
					}
					case GAME_RESET_MESSAGE_TYPE:
					{
						char buffer[] = "ng";
						sendData(gNetworkConnection->socket, buffer, sizeof(buffer) - 1, address);
						
						break;
					}
					case FIRST_SERVER_RESPONSE_MESSAGE_TYPE:
					{
						char buffer[256];
						snprintf(buffer, sizeof(buffer) - 1, "sr%d %f %f %d %d", message.firstServerResponse.slotID, message.firstServerResponse.x, message.firstServerResponse.y, message.firstServerResponse.direction, message.firstServerResponse.characterLives);
						
						sendData(gNetworkConnection->socket, buffer, strlen(buffer), address);
						
						break;
					}
					case FIRST_CLIENT_RESPONSE_MESSAGE_TYPE:
						break;
					case FIRST_DATA_TO_CLIENT_MESSAGE_TYPE:
					{
						int clientCharacterID = message.firstDataToClient.characterID;
						CharacterInfo *characterInfos = message.firstDataToClient.characterInfo;
						CharacterInfo *clientCharacterInfo = &characterInfos[clientCharacterID - 1];
						
						// send client its spawn info
						{
							GameMessage responseMessage;
							responseMessage.type = FIRST_SERVER_RESPONSE_MESSAGE_TYPE;
							responseMessage.firstServerResponse.slotID = clientCharacterID - 1;
							responseMessage.firstServerResponse.x = clientCharacterInfo->x;
							responseMessage.firstServerResponse.y = clientCharacterInfo->y;
							responseMessage.firstServerResponse.direction = clientCharacterInfo->direction;
							responseMessage.firstServerResponse.characterLives = gCharacterLives;
							
							responseMessage.addressIndex = message.addressIndex;
							pushNetworkMessage(&gGameMessagesToNet, responseMessage);
						}
						
						{
							// send all other clients the new client's net name
							GameMessage netNameMessage;
							netNameMessage.type = NET_NAME_MESSAGE_TYPE;
							netNameMessage.netNameRequest.characterID = clientCharacterID;
							netNameMessage.netNameRequest.netName = clientCharacterInfo->netName;
							
							sendToClients(message.addressIndex + 1, &netNameMessage);
							
							// also tell new client our net name
							netNameMessage.netNameRequest.characterID = PINK_BUBBLE_GUM;
							netNameMessage.addressIndex = message.addressIndex;
							pushNetworkMessage(&gGameMessagesToNet, netNameMessage);
						}
						
						// send client info to all other clients
						{
							GameMessage movedMessage;
							movedMessage.type = CHARACTER_MOVED_UPDATE_MESSAGE_TYPE;
							movedMessage.movedUpdate.characterID = clientCharacterID;
							movedMessage.movedUpdate.x = clientCharacterInfo->x;
							movedMessage.movedUpdate.y = clientCharacterInfo->y;
							movedMessage.movedUpdate.z = clientCharacterInfo->z;
							movedMessage.movedUpdate.direction = clientCharacterInfo->direction;
							
							sendToClients(message.addressIndex + 1, &movedMessage);
						}
						
						for (int characterIndex = RED_ROVER; characterIndex <= PINK_BUBBLE_GUM; characterIndex++)
						{
							if (characterIndex != clientCharacterID)
							{
								CharacterInfo *characterInfo = &characterInfos[characterIndex - 1];
								
								{
									GameMessage movementMessage;
									movementMessage.type = CHARACTER_MOVED_UPDATE_MESSAGE_TYPE;
									movementMessage.movedUpdate.characterID = characterIndex;
									movementMessage.movedUpdate.x = characterInfo->x;
									movementMessage.movedUpdate.y = characterInfo->y;
									movementMessage.movedUpdate.z = characterInfo->z;
									movementMessage.movedUpdate.direction = characterInfo->direction;
									
									movementMessage.addressIndex = message.addressIndex;
									pushNetworkMessage(&gGameMessagesToNet, movementMessage);
								}
								
								if (characterInfo->netName)
								{
									GameMessage netNameMessage;
									netNameMessage.type = NET_NAME_MESSAGE_TYPE;
									netNameMessage.netNameRequest.characterID = characterIndex;
									netNameMessage.netNameRequest.netName = characterInfo->netName;
									
									netNameMessage.addressIndex = message.addressIndex;
									pushNetworkMessage(&gGameMessagesToNet, netNameMessage);
								}
							}
						}
						
						int numPlayersToWaitFor = message.firstDataToClient.numberOfPlayersToWaitFor;
						if (numPlayersToWaitFor <= 0)
						{
							// tell all other clients the game has started
							GameMessage startedMessage;
							startedMessage.type = START_GAME_MESSAGE_TYPE;
							sendToClients(0, &startedMessage);
						}
						else
						{
							// tell clients how many players we are now waiting for
							
							GameMessage numberOfPlayersMessage;
							numberOfPlayersMessage.type = NUMBER_OF_PLAYERS_WAITING_FOR_MESSAGE_TYPE;
							numberOfPlayersMessage.numberOfWaitingPlayers = numPlayersToWaitFor;
							pushNetworkMessage(&gGameMessagesFromNet, numberOfPlayersMessage);
							
							sendToClients(0, &numberOfPlayersMessage);
						}
						
						break;
					}
				}
			}
			
			free(messagesAvailable);
			
			if (needsToQuit)
			{
				GameMessage quitMessage;
				quitMessage.type = QUIT_MESSAGE_TYPE;
				pushNetworkMessage(&gGameMessagesFromNet, quitMessage);
				
				break;
			}
		}
		
		fd_set socketSet;
		FD_ZERO(&socketSet);
		FD_SET(gNetworkConnection->socket, &socketSet);
		
		struct timeval waitValue;
		waitValue.tv_sec = 0;
		waitValue.tv_usec = 10000;
		
		if (select(gNetworkConnection->socket + 1, &socketSet, NULL, NULL, &waitValue) > 0)
		{
			char buffer[MAX_NETWORK_BUFFER];
			struct sockaddr_in address;
			unsigned int address_length = sizeof(address);
			int numberOfBytes;
			
			if ((numberOfBytes = recvfrom(gNetworkConnection->socket, buffer, MAX_NETWORK_BUFFER - 1, 0, (struct sockaddr *)&address, &address_length)) == -1)
			{
				// Ignore it and continue on
				fprintf(stderr, "recvfrom() actually returned -1\n");
			}
			else
			{
				buffer[numberOfBytes] = '\0';
				
				// can i play?
				if (buffer[0] == 'c' && buffer[1] == 'p')
				{
					if (numberOfPlayersToWaitFor > 0)
					{
						// yes
						// sr == server response
						gClientAddress[gCurrentSlot] = address;
						
						SDL_LockMutex(gCurrentSlotMutex);
						gCurrentSlot++;
						SDL_UnlockMutex(gCurrentSlotMutex);
						
						numberOfPlayersToWaitFor--;
						
						char *netName = malloc(MAX_USER_NAME_SIZE);
						memset(netName, 0, MAX_USER_NAME_SIZE);
						strncpy(netName, buffer + 2, MAX_USER_NAME_SIZE - 1);
						
						GameMessage message;
						message.type = FIRST_CLIENT_RESPONSE_MESSAGE_TYPE;
						message.addressIndex = gCurrentSlot - 1;
						message.firstClientResponse.netName = netName;
						message.firstClientResponse.numberOfPlayersToWaitFor = numberOfPlayersToWaitFor;
						message.firstClientResponse.slotID = gCurrentSlot;
						
						pushNetworkMessage(&gGameMessagesFromNet, message);
					}
					else
					{
						// no
						// sr == server response
						sendData(gNetworkConnection->socket, "srn", 3, &address);
					}
				}
				
				else if (buffer[0] == 'r' && buffer[1] == 'm')
				{
					// request movement
					
					int characterID = characterIDForClientAddress(&address);
					int direction = 0;
					
					sscanf(buffer + 2, "%d", &direction);
					
					if (direction == LEFT || direction == RIGHT || direction == UP || direction == DOWN || direction == NO_DIRECTION)
					{
						if (characterID > NO_CHARACTER && characterID <= PINK_BUBBLE_GUM)
						{
							GameMessage message;
							message.type = MOVEMENT_REQUEST_MESSAGE_TYPE;
							message.addressIndex = characterID - 1;
							message.movementRequest.direction = direction;
							
							pushNetworkMessage(&gGameMessagesFromNet, message);
						}
					}
				}
				
				else if (buffer[0] == 's' && buffer[1] == 'w')
				{
					// shoot weapon
					
					int characterID = characterIDForClientAddress(&address);
					if (characterID > NO_CHARACTER && characterID <= PINK_BUBBLE_GUM)
					{
						GameMessage message;
						message.type = CHARACTER_FIRED_MESSAGE_TYPE;
						message.firingRequest.characterID = characterID;
						
						pushNetworkMessage(&gGameMessagesFromNet, message);
					}
				}
				
				else if (buffer[0] == 'q' && buffer[1] == 'u')
				{
					GameMessage message;
					message.type = QUIT_MESSAGE_TYPE;
					
					int characterID = characterIDForClientAddress(&address);
					
					sendToClients(characterID, &message);
				}
			}
		}
	}
	
#ifdef WINDOWS
	WSACleanup();
#endif
	
	return 0;
}

int clientNetworkThread(void *context)
{
	// tell the server we exist
	GameMessage welcomeMessage;
	welcomeMessage.type = WELCOME_MESSAGE_TO_SERVER_MESSAGE_TYPE;
	welcomeMessage.weclomeMessage.netName = gUserNameString;
	sendToServer(welcomeMessage);
	
	SDL_bool receivedFirstServerMessage = SDL_FALSE;
	
	while (SDL_TRUE)
	{
		uint32_t messagesCount = 0;
		GameMessage *messagesAvailable = popNetworkMessages(&gGameMessagesToNet, &messagesCount);
		if (messagesAvailable != NULL)
		{
			SDL_bool needsToQuit = SDL_FALSE;
			
			for (uint32_t messageIndex = 0; messageIndex < messagesCount && !needsToQuit; messageIndex++)
			{
				GameMessage message = messagesAvailable[messageIndex];
				
				switch (message.type)
				{
					case WELCOME_MESSAGE_TO_SERVER_MESSAGE_TYPE:
					{
						char buffer[256];
						snprintf(buffer, sizeof(buffer) - 1, "cp%s", gUserNameString);
						
						sendData(gNetworkConnection->socket, buffer, sizeof(buffer) - 1, &gNetworkConnection->hostAddress);
						
						break;
					}
					case QUIT_MESSAGE_TYPE:
					{
						char buffer[] = "qu";
						
						sendData(gNetworkConnection->socket, buffer, sizeof(buffer) - 1, &gNetworkConnection->hostAddress);
						
						needsToQuit = SDL_TRUE;
						
						break;
					}
					case MOVEMENT_REQUEST_MESSAGE_TYPE:
					{
						char buffer[256];
						
						snprintf(buffer, sizeof(buffer) - 1, "rm%d", message.movementRequest.direction);
						
						sendData(gNetworkConnection->socket, buffer, strlen(buffer), &gNetworkConnection->hostAddress);
						
						break;
					}
					case CHARACTER_FIRED_MESSAGE_TYPE:
					{
						char buffer[] = "sw";
						
						sendData(gNetworkConnection->socket, buffer, sizeof(buffer) - 1, &gNetworkConnection->hostAddress);
						
						break;
					}
					case NUMBER_OF_PLAYERS_WAITING_FOR_MESSAGE_TYPE:
						break;
					case NET_NAME_MESSAGE_TYPE:
						break;
					case START_GAME_MESSAGE_TYPE:
						break;
					case GAME_START_NUMBER_UPDATE_MESSAGE_TYPE:
						break;
					case CHARACTER_DIED_UPDATE_MESSAGE_TYPE:
						break;
					case CHARACTER_MOVED_UPDATE_MESSAGE_TYPE:
						break;
					case CHARACTER_KILLED_UPDATE_MESSAGE_TYPE:
						break;
					case CHARACTER_SPAWNED_UPDATE_MESSAGE_TYPE:
						break;
					case GAME_RESET_MESSAGE_TYPE:
						break;
					case FIRST_SERVER_RESPONSE_MESSAGE_TYPE:
						break;
					case FIRST_CLIENT_RESPONSE_MESSAGE_TYPE:
						break;
					case FIRST_DATA_TO_CLIENT_MESSAGE_TYPE:
						break;
				}
			}
			
			free(messagesAvailable);
			
			if (needsToQuit)
			{
				GameMessage message;
				message.type = QUIT_MESSAGE_TYPE;
				
				pushNetworkMessage(&gGameMessagesFromNet, message);
				
				break;
			}
		}
		
		fd_set socketSet;
		FD_ZERO(&socketSet);
		FD_SET(gNetworkConnection->socket, &socketSet);
		
		struct timeval waitValue;
		waitValue.tv_sec = 0;
		waitValue.tv_usec = 10000;
		
		if (select(gNetworkConnection->socket + 1, &socketSet, NULL, NULL, &waitValue) > 0)
		{
			int numberOfBytes;
			char buffer[256];
			unsigned int address_length = sizeof(gNetworkConnection->hostAddress);
			
			if ((numberOfBytes = recvfrom(gNetworkConnection->socket, buffer, MAX_NETWORK_BUFFER - 1, 0, (struct sockaddr *)&gNetworkConnection->hostAddress, &address_length)) == -1)
			{
				fprintf(stderr, "recvfrom() actually returned -1\n");
			}
			else
			{
				buffer[numberOfBytes] = '\0';
				
				if (buffer[0] == 's' && buffer[1] == 'r')
				{
					if (buffer[2] == 'n')
					{
						GameMessage message;
						message.type = QUIT_MESSAGE_TYPE;
						pushNetworkMessage(&gGameMessagesFromNet, message);
						
						break;
					}
					else
					{
						int slotID = 0;
						float x = 0;
						float y = 0;
						int direction = NO_DIRECTION;
						int characterLives = 0;
						
						sscanf(buffer, "sr%d %f %f %d %d", &slotID, &x, &y, &direction, &characterLives);
						
						GameMessage message;
						message.type = FIRST_SERVER_RESPONSE_MESSAGE_TYPE;
						message.firstServerResponse.slotID = slotID;
						message.firstServerResponse.x = x;
						message.firstServerResponse.y = y;
						message.firstServerResponse.direction = direction;
						message.firstServerResponse.characterLives = characterLives;
						
						pushNetworkMessage(&gGameMessagesFromNet, message);
					}
					
					receivedFirstServerMessage = SDL_TRUE;
				}
				else if (buffer[0] == 'n' && buffer[1] == 'w')
				{
					int numberOfWaitingPlayers = 0;
					sscanf(buffer + 2, "%d", &numberOfWaitingPlayers);
					
					if (numberOfWaitingPlayers >= 0 && numberOfWaitingPlayers < 4)
					{
						GameMessage message;
						message.type = NUMBER_OF_PLAYERS_WAITING_FOR_MESSAGE_TYPE;
						message.numberOfWaitingPlayers = numberOfWaitingPlayers;
						pushNetworkMessage(&gGameMessagesFromNet, message);
					}
				}
				else if (buffer[0] == 'n' && buffer[1] == 'n')
				{
					// net name
					int characterID = 0;
					char *netName = malloc(MAX_USER_NAME_SIZE);
					if (netName != NULL)
					{
						memset(netName, 0, MAX_USER_NAME_SIZE);
						sscanf(buffer + 2, "%d %s", &characterID, netName);
						
						if (characterID > NO_CHARACTER && characterID <= PINK_BUBBLE_GUM)
						{
							GameMessage message;
							message.type = NET_NAME_MESSAGE_TYPE;
							message.netNameRequest.characterID = characterID;
							message.netNameRequest.netName = netName;
							pushNetworkMessage(&gGameMessagesFromNet, message);
						}
						else
						{
							free(netName);
						}
					}
				}
				else if (buffer[0] == 's' && buffer[1] == 'g')
				{
					// start game
					GameMessage message;
					message.type = START_GAME_MESSAGE_TYPE;
					pushNetworkMessage(&gGameMessagesFromNet, message);
				}
				else if (buffer[0] == 'g' && buffer[1] == 's')
				{
					int gameStartNumber;
					sscanf(buffer + 2, "%d", &gameStartNumber);
					
					GameMessage message;
					message.type = GAME_START_NUMBER_UPDATE_MESSAGE_TYPE;
					message.gameStartNumber = gameStartNumber;
					pushNetworkMessage(&gGameMessagesFromNet, message);
				}
				else if (receivedFirstServerMessage && buffer[0] == 'm' && buffer[1] == 'o')
				{
					int characterID = 0;
					int direction = 0;
					float x = 0.0;
					float y = 0.0;
					float z = 0.0;
					
					sscanf(buffer + 2, "%d %f %f %f %d", &characterID, &x, &y, &z, &direction);
					
					if (characterID > NO_CHARACTER && characterID <= PINK_BUBBLE_GUM)
					{
						GameMessage message;
						message.type = CHARACTER_MOVED_UPDATE_MESSAGE_TYPE;
						message.movedUpdate.characterID = characterID;
						message.movedUpdate.x = x;
						message.movedUpdate.y = y;
						message.movedUpdate.z = z;
						message.movedUpdate.direction = direction;
						pushNetworkMessage(&gGameMessagesFromNet, message);
					}
				}
				else if (buffer[0] == 'p' && buffer[1] == 'k')
				{
					// character gets killed
					
					int characterID = 0;
					int characterLives = 0;
					sscanf(buffer + 2, "%d %d", &characterID, &characterLives);
					
					if (characterID > NO_CHARACTER && characterID <= PINK_BUBBLE_GUM)
					{
						GameMessage message;
						message.type = CHARACTER_DIED_UPDATE_MESSAGE_TYPE;
						message.diedUpdate.characterID = characterID;
						message.diedUpdate.characterLives = characterLives;
						pushNetworkMessage(&gGameMessagesFromNet, message);
					}
				}
				else if (buffer[0] == 'c' && buffer[1] == 'k')
				{
					// character's kills increase
					
					int characterID = 0;
					int characterKills = 0;
					sscanf(buffer + 2, "%d %d", &characterID, &characterKills);
					
					if (characterID > NO_CHARACTER && characterID <= PINK_BUBBLE_GUM)
					{
						GameMessage message;
						message.type = CHARACTER_KILLED_UPDATE_MESSAGE_TYPE;
						message.killedUpdate.characterID = characterID;
						message.killedUpdate.kills = characterKills;
						pushNetworkMessage(&gGameMessagesFromNet, message);
					}
				}
				else if (buffer[0] =='s' && buffer[1] == 'p')
				{
					// spawn character
					int characterID = 0;
					float x = 0.0;
					float y = 0.0;
					
					sscanf(buffer + 2, "%d %f %f", &characterID, &x, &y);
					
					if (characterID > NO_CHARACTER && characterID <= PINK_BUBBLE_GUM)
					{
						GameMessage message;
						message.type = CHARACTER_SPAWNED_UPDATE_MESSAGE_TYPE;
						message.spawnedUpdate.characterID = characterID;
						message.spawnedUpdate.x = x;
						message.spawnedUpdate.y = y;
						pushNetworkMessage(&gGameMessagesFromNet, message);
					}
				}
				else if (buffer[0] == 's' && buffer[1] == 'w')
				{
					// shoot weapon
					int characterID = atoi(buffer + 2);
					if (characterID > NO_CHARACTER && characterID <= PINK_BUBBLE_GUM)
					{
						GameMessage message;
						message.type = CHARACTER_FIRED_MESSAGE_TYPE;
						message.firingRequest.characterID = characterID;
						
						pushNetworkMessage(&gGameMessagesFromNet, message);
					}
				}
				else if (buffer[0] == 'n' && buffer[1] == 'g')
				{
					// new game
					
					GameMessage message;
					message.type = GAME_RESET_MESSAGE_TYPE;
					
					pushNetworkMessage(&gGameMessagesFromNet, message);
				}
				else if (buffer[0] == 'q' && buffer[1] == 'u')
				{
					// quit
					GameMessage message;
					message.type = QUIT_MESSAGE_TYPE;
					pushNetworkMessage(&gGameMessagesFromNet, message);
					
					break;
				}
			}
		}
	}
	
	return 0;
}

static void initializeGameBuffer(GameMessageArray *messageArray)
{
	messageArray->messages = NULL;
	messageArray->count = 0;
	messageArray->mutex = SDL_CreateMutex();
}

void initializeNetworkBuffers(void)
{
	initializeGameBuffer(&gGameMessagesFromNet);
	initializeGameBuffer(&gGameMessagesToNet);
	
	gCurrentSlotMutex = SDL_CreateMutex();
}

#define MAX_MESSAGES_CAPACITY 1024
static void _pushNetworkMessage(GameMessageArray *messageArray, GameMessage message)
{
	SDL_bool appending = SDL_TRUE;
	
	if (messageArray->messages == NULL)
	{
		messageArray->messages = malloc(MAX_MESSAGES_CAPACITY * sizeof(message));
		if (messageArray->messages == NULL)
		{
			appending = SDL_FALSE;
		}
	}
	
	if (messageArray->count >= MAX_MESSAGES_CAPACITY)
	{
		appending = SDL_FALSE;
	}
	
	if (appending)
	{
		messageArray->messages[messageArray->count] = message;
		messageArray->count++;
	}
}

void pushNetworkMessage(GameMessageArray *messageArray, GameMessage message)
{
	SDL_LockMutex(messageArray->mutex);
	
	_pushNetworkMessage(messageArray, message);
	
	SDL_UnlockMutex(messageArray->mutex);
}

static void depleteNetworkMessages(GameMessageArray *messageArray)
{
	SDL_LockMutex(messageArray->mutex);
	
	messageArray->count = 0;
	
	SDL_UnlockMutex(messageArray->mutex);
}

GameMessage *popNetworkMessages(GameMessageArray *messageArray, uint32_t *count)
{
	GameMessage *newMessages = NULL;
	
	SDL_LockMutex(messageArray->mutex);
	
	if (messageArray->count > 0)
	{
		size_t size = sizeof(*newMessages) * messageArray->count;
		newMessages = malloc(size);
		if (newMessages != NULL)
		{
			memcpy(newMessages, messageArray->messages, size);
			*count = messageArray->count;
			messageArray->count = 0;
		}
	}
	
	SDL_UnlockMutex(messageArray->mutex);
	
	return newMessages;
}

void networkInitialization(void)
{
#ifdef WINDOWS
	WSADATA wsaData;
	
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
	{
		fprintf(stderr, "WSAStartup failed.\n");
    }
#endif
}

static void cleanUpNetName(Character *character)
{
	if (character->netName && character->netName != gUserNameString)
	{
		free(character->netName);
	}
	
	character->netName = NULL;
}

// Not to be called on network threads
void cleanupStateFromNetwork(void)
{
	restoreAllBackupStates();
	
	gPinkBubbleGumInput.character = &gPinkBubbleGum;
	gRedRoverInput.character = &gRedRover;
	gBlueLightningInput.character = &gBlueLightning;
	gGreenTreeInput.character = &gGreenTree;
	
	gPinkBubbleGum.netState = NETWORK_NO_STATE;
	gRedRover.netState = NETWORK_NO_STATE;
	gGreenTree.netState = NETWORK_NO_STATE;
	gBlueLightning.netState = NETWORK_NO_STATE;
	
	cleanUpNetName(&gPinkBubbleGum);
	cleanUpNetName(&gRedRover);
	cleanUpNetName(&gGreenTree);
	cleanUpNetName(&gBlueLightning);
}

void sendToClients(int exception, GameMessage *message)
{
	SDL_LockMutex(gCurrentSlotMutex);
	SDL_LockMutex(gGameMessagesToNet.mutex);
	
	for (int clientIndex = 0; clientIndex < gCurrentSlot; clientIndex++)
	{
		if (clientIndex + 1 != exception)
		{
			message->addressIndex = clientIndex;
			_pushNetworkMessage(&gGameMessagesToNet, *message);
		}
	}
	
	if (message->type == QUIT_MESSAGE_TYPE)
	{
		// Just add a quit message in case there's no clients we need to tell to quit
		message->addressIndex = -1;
		_pushNetworkMessage(&gGameMessagesToNet, *message);
	}
	
	SDL_UnlockMutex(gGameMessagesToNet.mutex);
	SDL_UnlockMutex(gCurrentSlotMutex);
}

void sendToServer(GameMessage message)
{
	pushNetworkMessage(&gGameMessagesToNet, message);
}

void closeSocket(int sockfd)
{
#ifdef WINDOWS
	closesocket(sockfd);
#else
	close(sockfd);
#endif
}
