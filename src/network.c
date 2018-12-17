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

const int NETWORK_SERVER_TYPE =			1;
const int NETWORK_CLIENT_TYPE =			2;
const Uint16 NETWORK_PORT =				4893;

#define MAX_NETWORK_BUFFER	256

NetworkConnection *gNetworkConnection = NULL;

struct sockaddr_in gClientAddress[3];
int gCurrentSlot = 0;

int serverNetworkThread(void *unused)
{
	while (gNetworkConnection && gNetworkConnection->shouldRun)
	{
		char buffer[MAX_NETWORK_BUFFER];
		struct sockaddr_in address;
		unsigned int address_length = sizeof(address);
		int numberOfBytes;
		
		fd_set socketSet;
		FD_ZERO(&socketSet);
		FD_SET(gNetworkConnection->socket, &socketSet);
		
		struct timeval waitValue;
		waitValue.tv_sec = 0;
		waitValue.tv_usec = 10000;
		
		if (select(gNetworkConnection->socket + 1, &socketSet, NULL, NULL, &waitValue) <= 0)
		{
			continue;
		}
		
		if ((numberOfBytes = recvfrom(gNetworkConnection->socket, buffer, MAX_NETWORK_BUFFER - 1, 0, (struct sockaddr *)&address, &address_length)) == -1)
		{
			// quit
			sendToClients(atoi(buffer + 2), "qu");
			
			closeSocket(gNetworkConnection->socket);
			free(gNetworkConnection);
			gNetworkConnection = NULL;
			
			endGame();
			closeGameResources();
			
			break;
		}
		else
		{
			buffer[numberOfBytes] = '\0';
			
			// can i play?
			if (buffer[0] == 'c' && buffer[1] == 'p')
			{
				if (gRedRover.netState == NETWORK_PENDING_STATE || gGreenTree.netState == NETWORK_PENDING_STATE || gBlueLightning.netState == NETWORK_PENDING_STATE)
				{
					// yes
					// sr == server response
					gClientAddress[gCurrentSlot] = address;
					gCurrentSlot++;
					
					Character *character = getCharacter(gCurrentSlot);
					
					// get the client's net name
					character->netName = malloc(MAX_USER_NAME_SIZE);
					memset(character->netName, 0, MAX_USER_NAME_SIZE);
					sscanf(buffer + 2, "%s", character->netName);
					
					// send all other clients the new client's net name
					sendToClients(gCurrentSlot, "nn%i %s", gCurrentSlot, character->netName);
										
					// send client its spawn info
					sprintf(buffer, "sr%i %f %f %i %i", gCurrentSlot - 1, character->x, character->y, character->direction, gCharacterLives);
					sendto(gNetworkConnection->socket, buffer, strlen(buffer), 0, (struct sockaddr *)&address, sizeof(address));
					
					// also tell new client our net name
					sprintf(buffer, "nn%i %s", PINK_BUBBLE_GUM, gPinkBubbleGum.netName);
					sendto(gNetworkConnection->socket, buffer, strlen(buffer), 0, (struct sockaddr *)&address, sizeof(address));
					
					// send client info to all other clients
					sendToClients(gCurrentSlot, "mo%i %f %f %f %i", gCurrentSlot, character->x, character->y, character->z, character->direction);
					
					// send all of the character's info to the new client
					int characterIndex = 0;
					
					for (characterIndex = RED_ROVER; characterIndex <= PINK_BUBBLE_GUM; characterIndex++)
					{
						if (characterIndex != gCurrentSlot)
						{
							character = getCharacter(characterIndex);
							sprintf(buffer, "mo%i %f %f %f %i", characterIndex, character->x, character->y, character->z, character->direction);
							
							sendto(gNetworkConnection->socket, buffer, strlen(buffer), 0, (struct sockaddr *)&address, sizeof(address));
							
							if (character->netName)
							{
								sprintf(buffer, "nn%i %s", IDOfCharacter(character), character->netName);
								sendto(gNetworkConnection->socket, buffer, strlen(buffer), 0, (struct sockaddr *)&address, sizeof(address));
							}
						}
					}
					
					character = getCharacter(gCurrentSlot);
					
					if ((gGreenTree.netState != NETWORK_PENDING_STATE || &gGreenTree == character) && (gBlueLightning.netState != NETWORK_PENDING_STATE || &gBlueLightning == character) && (gRedRover.netState != NETWORK_PENDING_STATE || &gRedRover == character))
					{
						// tell all other clients the game has started
						sendToClients(0, "sg");
					}
					else
					{
						// tell clients how many players we are now waiting for 
						gNetworkConnection->numberOfPlayersToWaitFor--;
						sendToClients(0, "nw%i", gNetworkConnection->numberOfPlayersToWaitFor);
					}
					
					character->netState = NETWORK_PLAYING_STATE;
				}
				else
				{
					// no
					// sr == server response
					sendto(gNetworkConnection->socket, "srn", 3, 0, (struct sockaddr *)&address, sizeof(address));
				}
			}
			
			else if (buffer[0] == 'r' && buffer[1] == 'm')
			{
				// request movement
				
				int characterID = 0;
				int direction = 0;
				
				sscanf(buffer + 2, "%i %i", &characterID, &direction);
				
				if (direction == LEFT || direction == RIGHT || direction == UP || direction == DOWN || direction == NO_DIRECTION)
				{
					Character *character = getCharacter(characterID);
					if (character->active)
					{
						character->direction = direction;
					}
				}
			}
			
			else if (buffer[0] == 's' && buffer[1] == 'w')
			{
				// shoot weapon
				
				int characterID = atoi(buffer + 2);
				shootCharacterWeapon(getCharacter(characterID));
			}
			
			else if (buffer[0] == 'q' && buffer[1] == 'u')
			{
				sendToClients(atoi(buffer + 2), "qu");
				
				closeSocket(gNetworkConnection->socket);
				free(gNetworkConnection);
				gNetworkConnection = NULL;
				
				endGame();
				closeGameResources();
			}
		}
	}
	
	if (gNetworkConnection && !gNetworkConnection->shouldRun)
	{
		closeSocket(gNetworkConnection->socket);
		free(gNetworkConnection);
		gNetworkConnection = NULL;
	}
	
	networkCleanup();
	
	restoreAllBackupStates();
	
	return 0;
}

int clientNetworkThread(void *context)
{
	// tell the server we exist
	char welcomeMessage[256];
	sprintf(welcomeMessage, "cp%s", gUserNameString);
	sendto(gNetworkConnection->socket, welcomeMessage, strlen(welcomeMessage), 0, (struct sockaddr *)&gNetworkConnection->hostAddress, sizeof(gNetworkConnection->hostAddress));
	
	fd_set socketSet;
	FD_ZERO(&socketSet);
	FD_SET(gNetworkConnection->socket, &socketSet);
	
	struct timeval waitTimeForFirstResponse;
	waitTimeForFirstResponse.tv_sec = 2;
	waitTimeForFirstResponse.tv_usec = 0;
	
	if (select(gNetworkConnection->socket + 1, &socketSet, NULL, NULL, &waitTimeForFirstResponse) > 0)
	{
		char buffer[256];
		unsigned int address_length = sizeof(gNetworkConnection->hostAddress);
		int numberOfBytes = recvfrom(gNetworkConnection->socket, buffer, MAX_NETWORK_BUFFER - 1, 0, (struct sockaddr *)&gNetworkConnection->hostAddress, &address_length);
		if (numberOfBytes > 0)
		{
			if (buffer[0] == 's' && buffer[1] == 'r')
			{
				if (buffer[2] == 'n')
				{
					closeSocket(gNetworkConnection->socket);
					free(gNetworkConnection);
					gNetworkConnection = NULL;
				}
				else
				{
					gNetworkConnection->isConnected = SDL_TRUE;
					
					int slotID = 0;
					float x = 0;
					float y = 0;
					int direction = NO_DIRECTION;
					
					sscanf(buffer, "sr%i %f %f %i %i", &slotID, &x, &y, &direction, &gNetworkConnection->characterLives);
					
					if (slotID + 1 == RED_ROVER)
					{
						gNetworkConnection->input = &gRedRoverInput;
						gRedRover.netName = gUserNameString;
					}
					else if (slotID + 1 == GREEN_TREE)
					{
						gNetworkConnection->input = &gGreenTreeInput;
						gGreenTree.netName = gUserNameString;
					}
					else if (slotID + 1 == BLUE_LIGHTNING)
					{
						gNetworkConnection->input = &gBlueLightningInput;
						gBlueLightning.netName = gUserNameString;
					}
					
					gPinkBubbleGumInput.character = gNetworkConnection->input->character;
					gRedRoverInput.character = gNetworkConnection->input->character;
					gBlueLightningInput.character = gNetworkConnection->input->character;
					gGreenTreeInput.character = gNetworkConnection->input->character;
					
					// server is pending
					gPinkBubbleGum.netState = NETWORK_PENDING_STATE;
					
					initGame();
					
					gPinkBubbleGum.lives = gNetworkConnection->characterLives;
					gRedRover.lives = gNetworkConnection->characterLives;
					gGreenTree.lives = gNetworkConnection->characterLives;
					gBlueLightning.lives = gNetworkConnection->characterLives;
					
					gNetworkConnection->input->character->x = x;
					gNetworkConnection->input->character->y = y;
					gNetworkConnection->input->character->direction = direction;
				}
			}
		}
		else
		{
			closeSocket(gNetworkConnection->socket);
			free(gNetworkConnection);
			gNetworkConnection = NULL;
		}
	}
	else
	{
		closeSocket(gNetworkConnection->socket);
		free(gNetworkConnection);
		gNetworkConnection = NULL;
	}
	
	while (gNetworkConnection && gNetworkConnection->shouldRun)
	{
		int numberOfBytes;
		char buffer[256];
		unsigned int address_length = sizeof(gNetworkConnection->hostAddress);
		
		FD_ZERO(&socketSet);
		FD_SET(gNetworkConnection->socket, &socketSet);
		
		if ((numberOfBytes = recvfrom(gNetworkConnection->socket, buffer, MAX_NETWORK_BUFFER - 1, 0, (struct sockaddr *)&gNetworkConnection->hostAddress, &address_length)) == -1)
		{
			// quit
			sendToClients(atoi(buffer + 2), "qu");
			
			closeSocket(gNetworkConnection->socket);
			free(gNetworkConnection);
			gNetworkConnection = NULL;
			
			endGame();
			closeGameResources();
		}
		else
		{
			buffer[numberOfBytes] = '\0';
			
			if (buffer[0] == 'n' && buffer[1] == 'w')
			{
				sscanf(buffer + 2, "%i", &(gNetworkConnection->numberOfPlayersToWaitFor)); 
			}
			else if (buffer[0] == 'n' && buffer[1] == 'n')
			{
				// net name
				int characterID;
				char *netName = malloc(MAX_USER_NAME_SIZE);
				memset(netName, 0, MAX_USER_NAME_SIZE);
				sscanf(buffer + 2, "%i %s", &characterID, netName);
				
				if (!getCharacter(characterID)->netName)
				{
					getCharacter(characterID)->netName = netName;
				}
				else
				{
					free(netName);
				}
			}
			else if (buffer[0] == 's' && buffer[1] == 'g')
			{
				// start game
				gPinkBubbleGum.netState = NETWORK_PLAYING_STATE;
			}
			else if (buffer[0] == 'g' && buffer[1] == 's')
			{
				int gameStartNumber;
				sscanf(buffer + 2, "%i", &gameStartNumber);
				
				gGameStartNumber = gameStartNumber;
			}
			else if (buffer[0] == 'm' && buffer[1] == 'o')
			{
				int characterID = 0;
				int direction = 0;
				float x = 0.0;
				float y = 0.0;
				float z = 0.0;
				
				sscanf(buffer + 2, "%i %f %f %f %i", &characterID, &x, &y, &z, &direction);
				
				Character *character = getCharacter(characterID);
				
				character->direction = direction;
				
				character->x = x;
				character->y = y;
				character->z = z;
			}
			else if (buffer[0] == 'p' && buffer[1] == 'k')
			{
				// character gets killed
				
				int characterID = 0;
				int characterLives = 0;
				sscanf(buffer + 2, "%i %i", &characterID, &characterLives);
				
				Character *character = getCharacter(characterID);
				character->lives = characterLives;
				
				// it doesn't matter which input we choose
				prepareCharactersDeath(character);
				decideWhetherToMakeAPlayerAWinner(character);
			}
			else if (buffer[0] == 'c' && buffer[1] == 'k')
			{
				// character's kills increase
				
				int characterID = 0;
				int characterKills = 0;
				sscanf(buffer + 2, "%i %i", &characterID, &characterKills);
				
				Character *character = getCharacter(characterID);
				character->kills = characterKills;
			}
			else if (buffer[0] =='s' && buffer[1] == 'p')
			{
				// spawn character
				Character *character = NULL;
				int characterID = 0;
				float x = 0.0;
				float y = 0.0;
				
				sscanf(buffer + 2, "%i %f %f", &characterID, &x, &y);
				
				character = getCharacter(characterID);
				
				character->z = 2.0f;
				character->x = x;
				character->y = y;
				
				character->active = SDL_TRUE;
			}
			else if (buffer[0] == 's' && buffer[1] == 'w')
			{
				// shoot weapon
				shootCharacterWeaponWithoutChecks(getCharacter(atoi(buffer + 2)));
			}
			else if (buffer[0] == 'n' && buffer[1] == 'g')
			{
				// new game
				gGameShouldReset = SDL_TRUE;
			}
			else if (buffer[0] == 'q' && buffer[1] == 'u')
			{
				// quit
				closeSocket(gNetworkConnection->socket);
				free(gNetworkConnection);
				gNetworkConnection = NULL;
				
				endGame();
				closeGameResources();
			}
		}
	}
	
	if (gNetworkConnection && !gNetworkConnection->shouldRun)
	{
		closeSocket(gNetworkConnection->socket);
		free(gNetworkConnection);
		gNetworkConnection = NULL;
	}
	
	networkCleanup();
	
	return 0;
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

void networkCleanup(void)
{
	gPinkBubbleGumInput.character = &gPinkBubbleGum;
	gRedRoverInput.character = &gRedRover;
	gBlueLightningInput.character = &gBlueLightning;
	gGreenTreeInput.character = &gGreenTree;
	
	gCurrentSlot = 0;
	
	gPinkBubbleGum.netState = NETWORK_NO_STATE;
	gRedRover.netState = NETWORK_NO_STATE;
	gGreenTree.netState = NETWORK_NO_STATE;
	gBlueLightning.netState = NETWORK_NO_STATE;
	
	cleanUpNetName(&gPinkBubbleGum);
	cleanUpNetName(&gRedRover);
	cleanUpNetName(&gGreenTree);
	cleanUpNetName(&gBlueLightning);
	
#ifdef WINDOWS
	WSACleanup();
#endif
}

void sendToClients(int exception, const char *format, ...)
{
	va_list ap;
	char buffer[256];
	int formatIndex;
	int bufferIndex = 0;
	int stringIndex;
	char *string;
	
	va_start(ap, format);
	
	for (formatIndex = 0; format[formatIndex] != '\0'; formatIndex++)
	{
		if (format[formatIndex] != '%')
		{
			buffer[bufferIndex] = format[formatIndex];
			bufferIndex++;
			continue;
		}
		
		formatIndex++;
		
		switch (format[formatIndex])
		{
			case 'c':
				buffer[bufferIndex] = va_arg(ap, int);
				break;
			case 'd':
				bufferIndex += sprintf(&buffer[bufferIndex], "%i", va_arg(ap, int)) - 1;
				break;
			case 'i':
				bufferIndex += sprintf(&buffer[bufferIndex], "%i", va_arg(ap, int)) - 1;
				break;
			case '%':
				buffer[bufferIndex] = '%';
				break;
			case 'f':
				bufferIndex += sprintf(&buffer[bufferIndex], "%f", va_arg(ap, double)) - 1;
				break;
			case 's':
				string = va_arg(ap, char *);
				
				for (stringIndex = 0; string[stringIndex] != '\0'; stringIndex++)
				{
					buffer[bufferIndex++] = string[stringIndex];
				}
				
				bufferIndex--;
				break;
		}
		
		bufferIndex++;
	}
	
	va_end(ap);
	
	int clientIndex;
	
	for (clientIndex = 0; clientIndex < gCurrentSlot; clientIndex++)
	{
		if (clientIndex + 1 != exception)
		{
			sendto(gNetworkConnection->socket, buffer, strlen(buffer), 0, (struct sockaddr *)&gClientAddress[clientIndex], sizeof(gClientAddress[clientIndex]));
		}
	}
}

void closeSocket(int sockfd)
{
#ifdef WINDOWS
	closesocket(sockfd);
#else
	close(sockfd);
#endif
}
