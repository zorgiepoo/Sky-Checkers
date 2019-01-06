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
#include "audio.h"

const Uint16 NETWORK_PORT =				4893;

#define MAX_NETWORK_BUFFER	256

NetworkConnection *gNetworkConnection = NULL;

struct sockaddr_in gClientAddress[3];

static SDL_mutex *gCurrentSlotMutex;

static void pushNetworkMessage(GameMessageArray *messageArray, GameMessage message);
static void depleteNetworkMessages(GameMessageArray *messageArray);

void setPredictedDirection(Character *character, int direction)
{
	character->predictedDirection = direction;
	character->predictedDirectionTime = SDL_GetTicks() + gNetworkConnection->averageIncomingMovementMessageTime;
}

// previousMovement->ticks <= renderTime < nextMovement->ticks
static void interpolateCharacter(Character *character, CharacterMovement *previousMovement, CharacterMovement *nextMovement, uint32_t renderTime)
{
	SDL_bool characterAlive = CHARACTER_IS_ALIVE(character);
	SDL_bool characterShouldBeAlive = CHARACTER_IS_ALIVE(previousMovement);
	if (characterAlive || characterShouldBeAlive)
	{
		if (characterShouldBeAlive && !characterAlive)
		{
			character->active = SDL_TRUE;
		}
		else if (!characterShouldBeAlive && characterAlive)
		{
			character->active = SDL_FALSE;
		}
		
		SDL_bool checkDiscrepancy = SDL_TRUE;
		// Alter the previous movement to our prediction
		if (character->predictedDirectionTime > 0 && character->predictedDirectionTime >= previousMovement->ticks)
		{
			previousMovement->direction = character->predictedDirection;
			if (character->predictedDirection != NO_DIRECTION)
			{
				previousMovement->pointing_direction = character->predictedDirection;
			}
			else
			{
				previousMovement->pointing_direction = character->pointing_direction;
			}
			
			character->movementConsumedCounter = 0;
			
			if (character->predictedDirectionTime < nextMovement->ticks)
			{
				character->predictedDirectionTime = 0;
			}
			
			checkDiscrepancy = SDL_FALSE;
		}
		
		if (characterShouldBeAlive != characterAlive)
		{
			character->x = previousMovement->x;
			character->y = previousMovement->y;
			
			character->xDiscrepancy = 0.0f;
			character->yDiscrepancy = 0.0f;
		}
		else
		{
			if (checkDiscrepancy)
			{
				if (character->direction == previousMovement->direction)
				{
					character->movementConsumedCounter++;
				}
				
				if (character->movementConsumedCounter >= 4)
				{
					// If the character is too far away, warp them back to a known previous movement
					// Otherwise interpolate the character to compensate for the difference
					float warpDiscrepancy = 3.0f;
					if (fabsf(character->x - previousMovement->x) >= warpDiscrepancy || fabsf(character->y - previousMovement->y) >= warpDiscrepancy)
					{
						character->x = previousMovement->x;
						character->y = previousMovement->y;
						
						character->xDiscrepancy = 0.0f;
						character->yDiscrepancy = 0.0f;
					}
					else
					{
						character->xDiscrepancy = previousMovement->x - character->x;
						character->yDiscrepancy = previousMovement->y - character->y;
					}
					
					character->movementConsumedCounter = 0;
				}
			}
			else
			{
				character->xDiscrepancy = 0.0f;
				character->yDiscrepancy = 0.0f;
			}
		}
		
		if (characterShouldBeAlive != characterAlive)
		{
			character->z = previousMovement->z;
		}
		
		character->direction = previousMovement->direction;
		character->pointing_direction = previousMovement->pointing_direction;
	}
}

void syncNetworkState(SDL_Window *window, float timeDelta)
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
				case ACK_MESSAGE_TYPE:
					break;
				case PING_MESSAGE_TYPE:
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
				case CHARACTER_FIRED_UPDATE_MESSAGE_TYPE:
				{
					int characterID = message.firedUpdate.characterID;
					Character *character = getCharacter(characterID);
					
					character->pointing_direction = message.firedUpdate.direction;
					prepareFiringCharacterWeapon(character, message.firedUpdate.x, message.firedUpdate.y);
					
					break;
				}
				case CHARACTER_FIRED_REQUEST_MESSAGE_TYPE:
				{
					int characterID = message.firedUpdate.characterID;
					Character *character = getCharacter(characterID);
					prepareFiringCharacterWeapon(character, character->x, character->y);
					
					break;
				}
				case COLOR_TILE_MESSAGE_TYPE:
				{
					int characterID = message.colorTile.characterID;
					int tileIndex = message.colorTile.tileIndex;
					Character *character = getCharacter(characterID);
					
					gTiles[tileIndex].red = character->weap->red;
					gTiles[tileIndex].green = character->weap->green;
					gTiles[tileIndex].blue = character->weap->blue;
					gTiles[tileIndex].coloredID = characterID;
					
					break;
				}
				case TILE_FALLING_DOWN_MESSAGE_TYPE:
				{
					int tileIndex = message.fallingTile.tileIndex;
					if (message.fallingTile.dead)
					{
						gTiles[tileIndex].isDead = SDL_TRUE;
					}
					else
					{
						gTiles[tileIndex].state = SDL_FALSE;
					}
					
					gTiles[tileIndex].z -= OBJECT_FALLING_STEP;
					
					if (((SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0) && gAudioEffectsFlag)
					{
						playTileFallingSound();
					}
					
					break;
				}
				case RECOVER_TILE_MESSAGE_TYPE:
				{
					recoverDestroyedTile(message.recoverTile.tileIndex);
					
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
				case PONG_MESSAGE_TYPE:
				{
					uint32_t currentTicks = SDL_GetTicks();
					uint32_t halfPingTime = (uint32_t)((currentTicks - message.pongTimestamp) / 2.0f);
					
					gNetworkConnection->incomingMovementMessageTimes[gNetworkConnection->incomingMovementMessageTimeIndex % sizeof(gNetworkConnection->incomingMovementMessageTimes) / sizeof(*gNetworkConnection->incomingMovementMessageTimes)] = halfPingTime;
					
					gNetworkConnection->incomingMovementMessageTimeIndex++;
					
					gNetworkConnection->averageIncomingMovementMessageTime = 0;
					uint32_t count = 0;
					
					for (uint32_t index = 0; index < sizeof(gNetworkConnection->incomingMovementMessageTimes) / sizeof(*gNetworkConnection->incomingMovementMessageTimes); index++)
					{
						if (gNetworkConnection->incomingMovementMessageTimes[index] != 0)
						{
							gNetworkConnection->averageIncomingMovementMessageTime += gNetworkConnection->incomingMovementMessageTimes[index];
							count++;
						}
					}
					
					if (count > 0)
					{
						gNetworkConnection->averageIncomingMovementMessageTime /= count;
					}
					else
					{
						gNetworkConnection->averageIncomingMovementMessageTime = 0;
					}
					
					break;
				}
				case CHARACTER_MOVED_UPDATE_MESSAGE_TYPE:
				{
					uint32_t currentTicks = SDL_GetTicks();
					
					if (!gGameShouldReset && currentTicks >= gNetworkConnection->averageIncomingMovementMessageTime)
					{
						SDL_bool shouldSetCharacterPosition = SDL_FALSE;
						if (gNetworkConnection->averageIncomingMovementMessageTime > 0)
						{
							CharacterMovement newMovement;
							newMovement.x = message.movedUpdate.x;
							newMovement.y = message.movedUpdate.y;
							newMovement.z = message.movedUpdate.z;
							newMovement.direction = message.movedUpdate.direction;
							newMovement.pointing_direction = message.movedUpdate.pointing_direction;
							newMovement.ticks = currentTicks - gNetworkConnection->averageIncomingMovementMessageTime;
							
							int characterIndex = message.movedUpdate.characterID - 1;
							
							gNetworkConnection->characterMovements[characterIndex][gNetworkConnection->characterMovementCounts[characterIndex] % CHARACTER_MOVEMENTS_CAPACITY] = newMovement;
							
							if (gNetworkConnection->characterMovementCounts[characterIndex] == 0)
							{
								shouldSetCharacterPosition = SDL_TRUE;
							}
							
							gNetworkConnection->characterMovementCounts[characterIndex]++;
						}
						else
						{
							shouldSetCharacterPosition = SDL_TRUE;
						}
						
						if (shouldSetCharacterPosition)
						{
							Character *character = getCharacter(message.movedUpdate.characterID);
							
							character->active = SDL_TRUE;
							character->x = message.movedUpdate.x;
							character->y = message.movedUpdate.y;
							character->z = message.movedUpdate.z;
							character->direction = message.movedUpdate.direction;
							character->pointing_direction = message.movedUpdate.pointing_direction;
						}
					}
					
					break;
				}
				case CHARACTER_KILLED_UPDATE_MESSAGE_TYPE:
				{
					Character *character = getCharacter(message.killedUpdate.characterID);
					character->kills = message.killedUpdate.kills;
					break;
				}
				case GAME_RESET_MESSAGE_TYPE:
					gGameShouldReset = SDL_TRUE;
					
					memset(gNetworkConnection->characterMovements, 0, sizeof(gNetworkConnection->characterMovements));
					memset(gNetworkConnection->characterMovementCounts, 0, sizeof(gNetworkConnection->characterMovementCounts));
					
					break;
				case FIRST_SERVER_RESPONSE_MESSAGE_TYPE:
				{
					int slotID = message.firstServerResponse.slotID;
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
					
					break;
				}
				case FIRST_CLIENT_RESPONSE_MESSAGE_TYPE:
				{
					Character *character = getCharacter(message.firstClientResponse.slotID);
					character->netName = message.firstClientResponse.netName;
					character->netState = NETWORK_PLAYING_STATE;
					
					GameMessage messageBack;
					messageBack.type = FIRST_DATA_TO_CLIENT_MESSAGE_TYPE;
					messageBack.packetNumber = 0;
					messageBack.addressIndex = message.addressIndex;
					messageBack.firstDataToClient.characterID = message.firstClientResponse.slotID;
					messageBack.firstDataToClient.numberOfPlayersToWaitFor = message.firstClientResponse.numberOfPlayersToWaitFor;
					
					for (int characterIndex = RED_ROVER; characterIndex <= PINK_BUBBLE_GUM; characterIndex++)
					{
						Character *character = getCharacter(characterIndex);
						messageBack.firstDataToClient.netNames[characterIndex - 1] = character->netName;
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
	
	if (gNetworkConnection != NULL && gNetworkConnection->type == NETWORK_CLIENT_TYPE)
	{
		uint32_t currentTime = SDL_GetTicks();
		if (currentTime > 0)
		{
			uint32_t renderTime = currentTime - (uint32_t)(3 * gNetworkConnection->averageIncomingMovementMessageTime);
			for (int characterID = RED_ROVER; characterID <= PINK_BUBBLE_GUM; characterID++)
			{
				int characterIndex = characterID - 1;

				// Find the two points that will tell us where the client was 100ms ago?
				uint32_t initialCount = gNetworkConnection->characterMovementCounts[characterIndex] < CHARACTER_MOVEMENTS_CAPACITY ? gNetworkConnection->characterMovementCounts[characterIndex] : CHARACTER_MOVEMENTS_CAPACITY;

				if (initialCount > 0)
				{
					uint32_t count = initialCount;
					uint32_t characterMovementIndex = gNetworkConnection->characterMovementCounts[characterIndex] - 1;
					uint32_t firstCharacterMovementIndex = characterMovementIndex;

					while (count > 0)
					{
						uint32_t wrappedCharacterMovementIndex = characterMovementIndex % CHARACTER_MOVEMENTS_CAPACITY;

						CharacterMovement previousMovement = gNetworkConnection->characterMovements[characterIndex][wrappedCharacterMovementIndex];

						if (previousMovement.ticks <= renderTime)
						{
							if (characterMovementIndex != firstCharacterMovementIndex)
							{
								CharacterMovement nextMovement = gNetworkConnection->characterMovements[characterIndex][(characterMovementIndex + 1) % CHARACTER_MOVEMENTS_CAPACITY];

								Character *character = getCharacter(characterID);
								
								interpolateCharacter(character, &previousMovement, &nextMovement, renderTime);
							}
							break;
						}

						characterMovementIndex--;
						count--;
					}
				}
			}
		}
		
		// Resolve position discrepanies
		for (int characterID = RED_ROVER; characterID <= PINK_BUBBLE_GUM; characterID++)
		{
			Character *character = getCharacter(characterID);
			float displacementAdjustment = timeDelta * INITIAL_CHARACTER_SPEED / 64.0f;
			
			if (fabsf(character->xDiscrepancy) < displacementAdjustment)
			{
				character->xDiscrepancy = 0.0f;
			}
			else
			{
				if (character->xDiscrepancy > 0.0f)
				{
					character->x += displacementAdjustment;
					character->xDiscrepancy -= displacementAdjustment;
				}
				else
				{
					character->x -= displacementAdjustment;
					character->xDiscrepancy += displacementAdjustment;
				}
			}
			
			if (fabsf(character->yDiscrepancy) < displacementAdjustment)
			{
				character->yDiscrepancy = 0.0f;
			}
			else
			{
				if (character->yDiscrepancy > 0.0f)
				{
					character->y += displacementAdjustment;
					character->yDiscrepancy -= displacementAdjustment;
				}
				else
				{
					character->y -= displacementAdjustment;
					character->yDiscrepancy += displacementAdjustment;
				}
			}
		}
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
	
	uint64_t triggerOutgoingPacketNumbers[] = {1, 1, 1};
	uint64_t realTimeOutgoingPacketNumbers[] = {1, 1, 1};
	
	uint64_t triggerIncomingPacketNumbers[] = {0, 0, 0};
	
	uint64_t receivedAckPacketNumbers[3][256] = {{0}, {0}, {0}};
	size_t receivedAckPacketsCapacity = sizeof(receivedAckPacketNumbers[0]) / sizeof(*receivedAckPacketNumbers[0]);
	uint64_t receivedAckPacketCount = 0;
	
	SDL_bool needsToQuit = SDL_FALSE;
	
	while (!needsToQuit)
	{
		uint32_t timeBeforeProcessing = SDL_GetTicks();
		
		uint32_t messagesCount = 0;
		GameMessage *messagesAvailable = popNetworkMessages(&gGameMessagesToNet, &messagesCount);
		
		if (messagesAvailable != NULL)
		{
			char sendBuffers[3][4096];
			char *sendBufferPtrs[] = {sendBuffers[0], sendBuffers[1], sendBuffers[2]};
			
			// Only keep one movement message per character per packet
			uint32_t trackedMovementIndices[3][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};
			for (uint32_t messagesLeft = messagesCount; messagesLeft > 0; messagesLeft--)
			{
				GameMessage message = messagesAvailable[messagesLeft - 1];
				if (message.type == CHARACTER_MOVED_UPDATE_MESSAGE_TYPE)
				{
					int addressIndex = message.addressIndex;
					int characterIndex = message.movedUpdate.characterID - 1;
					
					if (trackedMovementIndices[addressIndex][characterIndex] == 0)
					{
						trackedMovementIndices[addressIndex][characterIndex] = messagesLeft - 1;
					}
					else
					{
						messagesAvailable[messagesLeft - 1].addressIndex = -1;
					}
				}
			}
			
			for (uint32_t messageIndex = 0; messageIndex < messagesCount; messageIndex++)
			{
				GameMessage message = messagesAvailable[messageIndex];
				int addressIndex = message.addressIndex;
				struct sockaddr_in *address = (addressIndex == -1) ? NULL :  &gClientAddress[addressIndex];
				
				if (!needsToQuit && message.type != QUIT_MESSAGE_TYPE && message.type != ACK_MESSAGE_TYPE && message.type != FIRST_DATA_TO_CLIENT_MESSAGE_TYPE && message.type != PONG_MESSAGE_TYPE)
				{
					if (message.packetNumber == 0)
					{
						if (message.type == CHARACTER_MOVED_UPDATE_MESSAGE_TYPE)
						{
							if (addressIndex == -1)
							{
								continue;
							}
							else
							{
								message.packetNumber = realTimeOutgoingPacketNumbers[addressIndex];
								realTimeOutgoingPacketNumbers[addressIndex]++;
							}
						}
						else
						{
							message.packetNumber = triggerOutgoingPacketNumbers[addressIndex];
							triggerOutgoingPacketNumbers[addressIndex]++;
							
							// Re-send message until we receive an ack
							pushNetworkMessage(&gGameMessagesToNet, message);
						}
					}
					else if (message.type != CHARACTER_MOVED_UPDATE_MESSAGE_TYPE)
					{
						SDL_bool foundAck = SDL_FALSE;
						uint64_t maxPacketCount = receivedAckPacketCount < receivedAckPacketsCapacity ? receivedAckPacketCount : receivedAckPacketsCapacity;
						for (uint64_t packetIndex = 0; packetIndex < maxPacketCount; packetIndex++)
						{
							if (message.packetNumber == receivedAckPacketNumbers[addressIndex][packetIndex])
							{
								foundAck = SDL_TRUE;
								break;
							}
						}
						
						if (foundAck)
						{
							continue;
						}
						else
						{
							// Re-send message until we receive an ack
							pushNetworkMessage(&gGameMessagesToNet, message);
						}
					}
				}
				
				if (needsToQuit && message.type != QUIT_MESSAGE_TYPE)
				{
					continue;
				}
				
				switch (message.type)
				{
					case WELCOME_MESSAGE_TO_SERVER_MESSAGE_TYPE:
						break;
					case PING_MESSAGE_TYPE:
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
						break;
					case CHARACTER_FIRED_UPDATE_MESSAGE_TYPE:
					{
						int length = snprintf(sendBufferPtrs[addressIndex], 256, "sw%llu %d %f %f %d", message.packetNumber, message.firedUpdate.characterID, message.firedUpdate.x, message.firedUpdate.y, message.firedUpdate.direction);
						
						sendBufferPtrs[addressIndex] += length + 1;
						
						if ((size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]) >= sizeof(sendBuffers[addressIndex]) - 256)
						{
							sendData(gNetworkConnection->socket, sendBuffers[addressIndex], (size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]), address);
							sendBufferPtrs[addressIndex] = sendBuffers[addressIndex];
						}
						
						break;
					}
					case CHARACTER_FIRED_REQUEST_MESSAGE_TYPE:
						break;
					case NUMBER_OF_PLAYERS_WAITING_FOR_MESSAGE_TYPE:
					{
						int length = snprintf(sendBufferPtrs[addressIndex], 256, "nw%llu %d", message.packetNumber, message.numberOfWaitingPlayers);
						
						sendBufferPtrs[addressIndex] += length + 1;
						
						if ((size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]) >= sizeof(sendBuffers[addressIndex]) - 256)
						{
							sendData(gNetworkConnection->socket, sendBuffers[addressIndex], (size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]), address);
							sendBufferPtrs[addressIndex] = sendBuffers[addressIndex];
						}
						
						break;
					}
					case NET_NAME_MESSAGE_TYPE:
					{
						int length = snprintf(sendBufferPtrs[addressIndex], 256, "nn%llu %d %s", message.packetNumber, message.netNameRequest.characterID, message.netNameRequest.netName);
						
						sendBufferPtrs[addressIndex] += length + 1;
						
						if ((size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]) >= sizeof(sendBuffers[addressIndex]) - 256)
						{
							sendData(gNetworkConnection->socket, sendBuffers[addressIndex], (size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]), address);
							sendBufferPtrs[addressIndex] = sendBuffers[addressIndex];
						}
						
						break;
					}
					case START_GAME_MESSAGE_TYPE:
					{
						int length = snprintf(sendBufferPtrs[addressIndex], 256, "sg%llu", message.packetNumber);
						
						sendBufferPtrs[addressIndex] += length + 1;
						
						if ((size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]) >= sizeof(sendBuffers[addressIndex]) - 256)
						{
							sendData(gNetworkConnection->socket, sendBuffers[addressIndex], (size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]), address);
							sendBufferPtrs[addressIndex] = sendBuffers[addressIndex];
						}
						
						break;
					}
					case GAME_START_NUMBER_UPDATE_MESSAGE_TYPE:
					{
						int length = snprintf(sendBufferPtrs[addressIndex], 256, "gs%llu %d", message.packetNumber, message.gameStartNumber);
						
						sendBufferPtrs[addressIndex] += length + 1;
						
						if ((size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]) >= sizeof(sendBuffers[addressIndex]) - 256)
						{
							sendData(gNetworkConnection->socket, sendBuffers[addressIndex], (size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]), address);
							sendBufferPtrs[addressIndex] = sendBuffers[addressIndex];
						}
						
						break;
					}
					case CHARACTER_DIED_UPDATE_MESSAGE_TYPE:
					{
						int length = snprintf(sendBufferPtrs[addressIndex], 256, "pk%llu %d %d", message.packetNumber, message.diedUpdate.characterID, message.diedUpdate.characterLives);
						
						sendBufferPtrs[addressIndex] += length + 1;
						
						if ((size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]) >= sizeof(sendBuffers[addressIndex]) - 256)
						{
							sendData(gNetworkConnection->socket, sendBuffers[addressIndex], (size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]), address);
							sendBufferPtrs[addressIndex] = sendBuffers[addressIndex];
						}
						
						break;
					}
					case COLOR_TILE_MESSAGE_TYPE:
					{
						int length = snprintf(sendBufferPtrs[addressIndex], 256, "ct%llu %d %d", message.packetNumber, message.colorTile.characterID, message.colorTile.tileIndex);
						
						sendBufferPtrs[addressIndex] += length + 1;
						
						if ((size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]) >= sizeof(sendBuffers[addressIndex]) - 256)
						{
							sendData(gNetworkConnection->socket, sendBuffers[addressIndex], (size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]), address);
							sendBufferPtrs[addressIndex] = sendBuffers[addressIndex];
						}
						
						break;
					}
					case TILE_FALLING_DOWN_MESSAGE_TYPE:
					{
						int length = snprintf(sendBufferPtrs[addressIndex], 256, "tf%llu %d %d", message.packetNumber, message.fallingTile.tileIndex, message.fallingTile.dead);
						
						sendBufferPtrs[addressIndex] += length + 1;
						
						if ((size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]) >= sizeof(sendBuffers[addressIndex]) - 256)
						{
							sendData(gNetworkConnection->socket, sendBuffers[addressIndex], (size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]), address);
							sendBufferPtrs[addressIndex] = sendBuffers[addressIndex];
						}
						
						break;
					}
					case RECOVER_TILE_MESSAGE_TYPE:
					{
						int length = snprintf(sendBufferPtrs[addressIndex], 256, "rt%llu %d", message.packetNumber, message.recoverTile.tileIndex);
						
						sendBufferPtrs[addressIndex] += length + 1;
						
						if ((size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]) >= sizeof(sendBuffers[addressIndex]) - 256)
						{
							sendData(gNetworkConnection->socket, sendBuffers[addressIndex], (size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]), address);
							sendBufferPtrs[addressIndex] = sendBuffers[addressIndex];
						}
						
						break;
					}
					case CHARACTER_MOVED_UPDATE_MESSAGE_TYPE:
					{
						int length = snprintf(sendBufferPtrs[addressIndex], 256, "mo%llu %d %f %f %f %d %d %u", message.packetNumber, message.movedUpdate.characterID, message.movedUpdate.x, message.movedUpdate.y, message.movedUpdate.z, message.movedUpdate.direction, message.movedUpdate.pointing_direction, message.movedUpdate.timestamp);
						
						sendBufferPtrs[addressIndex] += length + 1;
						
						if ((size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]) >= sizeof(sendBuffers[addressIndex]) - 256)
						{
							sendData(gNetworkConnection->socket, sendBuffers[addressIndex], (size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]), address);
							sendBufferPtrs[addressIndex] = sendBuffers[addressIndex];
						}
						
						break;
					}
					case CHARACTER_KILLED_UPDATE_MESSAGE_TYPE:
					{
						int length = snprintf(sendBufferPtrs[addressIndex], 256, "ck%llu %d %d", message.packetNumber, message.killedUpdate.characterID, message.killedUpdate.kills);
						
						sendBufferPtrs[addressIndex] += length + 1;
						
						if ((size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]) >= sizeof(sendBuffers[addressIndex]) - 256)
						{
							sendData(gNetworkConnection->socket, sendBuffers[addressIndex], (size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]), address);
							sendBufferPtrs[addressIndex] = sendBuffers[addressIndex];
						}
						
						break;
					}
					case GAME_RESET_MESSAGE_TYPE:
					{
						int length = snprintf(sendBufferPtrs[addressIndex], sizeof(sendBufferPtrs[addressIndex]) - 1, "ng%llu", message.packetNumber);
						
						sendBufferPtrs[addressIndex] += length + 1;
						
						if ((size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]) >= sizeof(sendBuffers[addressIndex]) - 256)
						{
							sendData(gNetworkConnection->socket, sendBuffers[addressIndex], (size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]), address);
							sendBufferPtrs[addressIndex] = sendBuffers[addressIndex];
						}
						
						break;
					}
					case ACK_MESSAGE_TYPE:
					{
						int length = snprintf(sendBufferPtrs[addressIndex], 256, "ak%llu", message.packetNumber);
						
						sendBufferPtrs[addressIndex] += length + 1;
						
						if ((size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]) >= sizeof(sendBuffers[addressIndex]) - 256)
						{
							sendData(gNetworkConnection->socket, sendBuffers[addressIndex], (size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]), address);
							sendBufferPtrs[addressIndex] = sendBuffers[addressIndex];
						}
						
						break;
					}
					case PONG_MESSAGE_TYPE:
					{
						int length = snprintf(sendBufferPtrs[addressIndex], 256, "po%u", message.pongTimestamp);
						
						sendBufferPtrs[addressIndex] += length + 1;
						
						if ((size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]) >= sizeof(sendBuffers[addressIndex]) - 256)
						{
							sendData(gNetworkConnection->socket, sendBuffers[addressIndex], (size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]), address);
							sendBufferPtrs[addressIndex] = sendBuffers[addressIndex];
						}
						
						break;
					}
					case FIRST_SERVER_RESPONSE_MESSAGE_TYPE:
					{
						int length = snprintf(sendBufferPtrs[addressIndex], 256, "sr%llu %d %d", message.packetNumber, message.firstServerResponse.slotID, message.firstServerResponse.characterLives);
						
						sendBufferPtrs[addressIndex] += length + 1;
						
						if ((size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]) >= sizeof(sendBuffers[addressIndex]) - 256)
						{
							sendData(gNetworkConnection->socket, sendBuffers[addressIndex], (size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]), address);
							sendBufferPtrs[addressIndex] = sendBuffers[addressIndex];
						}
						
						break;
					}
					case FIRST_CLIENT_RESPONSE_MESSAGE_TYPE:
						break;
					case FIRST_DATA_TO_CLIENT_MESSAGE_TYPE:
					{
						int clientCharacterID = message.firstDataToClient.characterID;
						
						// send client its initial info
						{
							GameMessage responseMessage;
							responseMessage.type = FIRST_SERVER_RESPONSE_MESSAGE_TYPE;
							responseMessage.packetNumber = 0;
							responseMessage.firstServerResponse.slotID = clientCharacterID - 1;
							responseMessage.firstServerResponse.characterLives = gCharacterLives;
							
							responseMessage.addressIndex = message.addressIndex;
							pushNetworkMessage(&gGameMessagesToNet, responseMessage);
						}
						
						{
							// send all other clients the new client's net name
							GameMessage netNameMessage;
							netNameMessage.type = NET_NAME_MESSAGE_TYPE;
							netNameMessage.packetNumber = 0;
							netNameMessage.netNameRequest.characterID = clientCharacterID;
							netNameMessage.netNameRequest.netName = message.firstDataToClient.netNames[clientCharacterID - 1];
							
							sendToClients(message.addressIndex + 1, &netNameMessage);
							
							// also tell new client our net name
							netNameMessage.netNameRequest.characterID = PINK_BUBBLE_GUM;
							netNameMessage.addressIndex = message.addressIndex;
							pushNetworkMessage(&gGameMessagesToNet, netNameMessage);
						}
						
						for (int characterIndex = RED_ROVER; characterIndex <= PINK_BUBBLE_GUM; characterIndex++)
						{
							if (characterIndex != clientCharacterID)
							{
								char *netName = message.firstDataToClient.netNames[characterIndex - 1];
								if (netName != NULL)
								{
									GameMessage netNameMessage;
									netNameMessage.type = NET_NAME_MESSAGE_TYPE;
									netNameMessage.packetNumber = 0;
									netNameMessage.netNameRequest.characterID = characterIndex;
									netNameMessage.netNameRequest.netName = netName;
									
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
			
			for (int addressIndex = 0; addressIndex < 3; addressIndex++)
			{
				if ((size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]) > 0)
				{
					sendData(gNetworkConnection->socket, sendBuffers[addressIndex], (size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]), &gClientAddress[addressIndex]);
				}
			}
			
			free(messagesAvailable);
			
			if (needsToQuit)
			{
				GameMessage quitMessage;
				quitMessage.type = QUIT_MESSAGE_TYPE;
				pushNetworkMessage(&gGameMessagesFromNet, quitMessage);
			}
		}
		
		while (!needsToQuit)
		{
			fd_set socketSet;
			FD_ZERO(&socketSet);
			FD_SET(gNetworkConnection->socket, &socketSet);
			
			struct timeval waitValue;
			waitValue.tv_sec = 0;
			waitValue.tv_usec = 0;
			
			if (select(gNetworkConnection->socket + 1, &socketSet, NULL, NULL, &waitValue) <= 0)
			{
				break;
			}
			else
			{
				char packetBuffer[4096];
				struct sockaddr_in address;
				unsigned int addressLength = sizeof(address);
				int numberOfBytes = 0;
				
				if ((numberOfBytes = recvfrom(gNetworkConnection->socket, packetBuffer, sizeof(packetBuffer), 0, (struct sockaddr *)&address, &addressLength)) == -1)
				{
					// Ignore it and continue on
					fprintf(stderr, "recvfrom() actually returned -1\n");
				}
				else
				{
					char *buffer = packetBuffer;
					while (buffer < packetBuffer + numberOfBytes)
					{
						// can i play?
						if (buffer[0] == 'c' && buffer[1] == 'p')
						{
							char *netName = malloc(MAX_USER_NAME_SIZE);
							memset(netName, 0, MAX_USER_NAME_SIZE);
							
							uint64_t packetNumber = 0;
							sscanf(buffer + 2, "%llu %s", &packetNumber, netName);
							
							int existingCharacterID = characterIDForClientAddress(&address);
							if ((packetNumber == 1 && existingCharacterID == NO_CHARACTER && numberOfPlayersToWaitFor > 0) || (packetNumber == 1 && existingCharacterID != NO_CHARACTER))
							{
								int addressIndex;
								if (existingCharacterID == NO_CHARACTER)
								{
									// yes
									// sr == server response
									addressIndex = gCurrentSlot;
									gClientAddress[addressIndex] = address;
									triggerIncomingPacketNumbers[addressIndex]++;
									
									SDL_LockMutex(gCurrentSlotMutex);
									gCurrentSlot++;
									SDL_UnlockMutex(gCurrentSlotMutex);
									
									numberOfPlayersToWaitFor--;
									
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
									addressIndex = existingCharacterID - 1;
								}
								
								if (packetNumber <= triggerIncomingPacketNumbers[addressIndex])
								{
									GameMessage ackMessage;
									ackMessage.type = ACK_MESSAGE_TYPE;
									ackMessage.packetNumber = packetNumber;
									ackMessage.addressIndex = addressIndex;
									pushNetworkMessage(&gGameMessagesToNet, ackMessage);
								}
							}
							else if (existingCharacterID == NO_CHARACTER)
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
							int addressIndex = characterID - 1;
							int direction = 0;
							uint64_t packetNumber = 0;
							
							sscanf(buffer + 2, "%llu %d", &packetNumber, &direction);
							
							if (packetNumber == triggerIncomingPacketNumbers[addressIndex] + 1)
							{
								triggerIncomingPacketNumbers[addressIndex]++;
								
								if (direction == LEFT || direction == RIGHT || direction == UP || direction == DOWN || direction == NO_DIRECTION)
								{
									if (characterID > NO_CHARACTER && characterID <= PINK_BUBBLE_GUM)
									{
										GameMessage message;
										message.type = MOVEMENT_REQUEST_MESSAGE_TYPE;
										message.addressIndex = addressIndex;
										message.movementRequest.direction = direction;
										
										pushNetworkMessage(&gGameMessagesFromNet, message);
									}
								}
							}
							
							if (packetNumber <= triggerIncomingPacketNumbers[addressIndex])
							{
								GameMessage ackMessage;
								ackMessage.type = ACK_MESSAGE_TYPE;
								ackMessage.packetNumber = packetNumber;
								ackMessage.addressIndex = addressIndex;
								pushNetworkMessage(&gGameMessagesToNet, ackMessage);
							}
						}
						
						else if (buffer[0] == 's' && buffer[1] == 'w')
						{
							// shoot weapon
							uint64_t packetNumber = 0;
							sscanf(buffer + 2, "%llu", &packetNumber);
							
							int characterID = characterIDForClientAddress(&address);
							int addressIndex = characterID - 1;
							
							if (packetNumber == triggerIncomingPacketNumbers[addressIndex] + 1)
							{
								triggerIncomingPacketNumbers[addressIndex]++;
								
								if (characterID > NO_CHARACTER && characterID <= PINK_BUBBLE_GUM)
								{
									GameMessage message;
									message.type = CHARACTER_FIRED_REQUEST_MESSAGE_TYPE;
									message.firedRequest.characterID = characterID;
									
									pushNetworkMessage(&gGameMessagesFromNet, message);
								}
							}
							
							if (packetNumber <= triggerIncomingPacketNumbers[addressIndex])
							{
								GameMessage ackMessage;
								ackMessage.type = ACK_MESSAGE_TYPE;
								ackMessage.packetNumber = packetNumber;
								ackMessage.addressIndex = addressIndex;
								pushNetworkMessage(&gGameMessagesToNet, ackMessage);
							}
						}
						
						else if (buffer[0] == 'a' && buffer[1] == 'k')
						{
							uint64_t packetNumber = 0;
							sscanf(buffer + 2, "%llu", &packetNumber);
							
							int addressIndex = characterIDForClientAddress(&address) - 1;
							
							SDL_bool foundAck = SDL_FALSE;
							uint64_t maxPacketCount = receivedAckPacketCount < receivedAckPacketsCapacity ? receivedAckPacketCount : receivedAckPacketsCapacity;
							for (uint64_t packetIndex = 0; packetIndex < maxPacketCount; packetIndex++)
							{
								if (packetNumber == receivedAckPacketNumbers[addressIndex][packetIndex])
								{
									foundAck = SDL_TRUE;
									break;
								}
							}
							if (!foundAck)
							{
								receivedAckPacketNumbers[addressIndex][receivedAckPacketCount % receivedAckPacketsCapacity] = packetNumber;
								receivedAckPacketCount++;
							}
						}
						
						else if (buffer[0] == 'p' && buffer[1] == 'i')
						{
							uint32_t timestamp = 0;
							sscanf(buffer + 2, "%u", &timestamp);
							
							int addressIndex = characterIDForClientAddress(&address) - 1;
							
							GameMessage pongMessage;
							pongMessage.type = PONG_MESSAGE_TYPE;
							pongMessage.addressIndex = addressIndex;
							pongMessage.pongTimestamp = timestamp;
							pushNetworkMessage(&gGameMessagesToNet, pongMessage);
						}
						
						else if (buffer[0] == 'q' && buffer[1] == 'u')
						{
							GameMessage message;
							message.type = QUIT_MESSAGE_TYPE;
							
							int characterID = characterIDForClientAddress(&address);
							
							sendToClients(characterID, &message);
						}
						
						buffer += strlen(buffer) + 1;
					}
				}
			}
		}
		
		if (!needsToQuit)
		{
			uint32_t timeAfterProcessing = SDL_GetTicks();
			uint32_t deltaProcessingTime = timeAfterProcessing - timeBeforeProcessing;
			const uint32_t targetDelay = 5;
			
			if (deltaProcessingTime < targetDelay)
			{
				SDL_Delay(targetDelay - deltaProcessingTime);
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
	uint64_t triggerOutgoingPacketNumber = 1;
	
	uint64_t triggerIncomingPacketNumber = 0;
	uint64_t realTimeIncomingPacketNumber = 0;
	
	uint64_t receivedAckPacketNumbers[256] = {0};
	size_t receivedAckPacketsCapacity = sizeof(receivedAckPacketNumbers) / sizeof(*receivedAckPacketNumbers);
	uint64_t receivedAckPacketCount = 0;
	
	// tell the server we exist
	GameMessage welcomeMessage;
	welcomeMessage.type = WELCOME_MESSAGE_TO_SERVER_MESSAGE_TYPE;
	welcomeMessage.weclomeMessage.netName = gUserNameString;
	sendToServer(welcomeMessage);
	
	SDL_bool needsToQuit = SDL_FALSE;
	
	while (!needsToQuit)
	{
		uint32_t timeBeforeProcessing = SDL_GetTicks();
		
		uint32_t messagesCount = 0;
		GameMessage *messagesAvailable = popNetworkMessages(&gGameMessagesToNet, &messagesCount);
		if (messagesAvailable != NULL)
		{
			char sendBuffer[4096];
			char *sendBufferPtr = sendBuffer;
			
			for (uint32_t messageIndex = 0; messageIndex < messagesCount && !needsToQuit; messageIndex++)
			{
				GameMessage message = messagesAvailable[messageIndex];
				if (message.type != QUIT_MESSAGE_TYPE && message.type != ACK_MESSAGE_TYPE && message.type != PING_MESSAGE_TYPE)
				{
					if (message.packetNumber == 0)
					{
						message.packetNumber = triggerOutgoingPacketNumber;
						triggerOutgoingPacketNumber++;
						
						// Re-send message until we receive an ack
						pushNetworkMessage(&gGameMessagesToNet, message);
					}
					else
					{
						SDL_bool foundAck = SDL_FALSE;
						uint64_t maxPacketCount = receivedAckPacketCount < receivedAckPacketsCapacity ? receivedAckPacketCount + 1 : receivedAckPacketsCapacity;
						for (uint64_t packetIndex = 0; packetIndex < maxPacketCount; packetIndex++)
						{
							if (message.packetNumber == receivedAckPacketNumbers[packetIndex])
							{
								foundAck = SDL_TRUE;
								break;
							}
						}
						
						if (foundAck)
						{
							continue;
						}
						else
						{
							// Re-send message until we receive an ack
							pushNetworkMessage(&gGameMessagesToNet, message);
						}
					}
				}
				
				switch (message.type)
				{
					case WELCOME_MESSAGE_TO_SERVER_MESSAGE_TYPE:
					{
						int length = snprintf(sendBufferPtr, 256, "cp%llu %s", message.packetNumber, gUserNameString);
						
						sendBufferPtr += length + 1;
						
						if ((size_t)(sendBufferPtr - sendBuffer) >= sizeof(sendBuffer) - 256)
						{
							sendData(gNetworkConnection->socket, sendBuffer, (size_t)(sendBufferPtr - sendBuffer), &gNetworkConnection->hostAddress);
							sendBufferPtr = sendBuffer;
						}
						
						break;
					}
					case QUIT_MESSAGE_TYPE:
					{
						char buffer[] = "qu";
						
						sendData(gNetworkConnection->socket, buffer, sizeof(buffer) - 1, &gNetworkConnection->hostAddress);
						
						needsToQuit = SDL_TRUE;
						
						break;
					}
					case PING_MESSAGE_TYPE:
					{
						int length = snprintf(sendBufferPtr, 256, "pi%u", message.pingTimestamp);
						
						sendBufferPtr += length + 1;
						
						if ((size_t)(sendBufferPtr - sendBuffer) >= sizeof(sendBuffer) - 256)
						{
							sendData(gNetworkConnection->socket, sendBuffer, (size_t)(sendBufferPtr - sendBuffer), &gNetworkConnection->hostAddress);
							sendBufferPtr = sendBuffer;
						}
						
						break;
					}
					case MOVEMENT_REQUEST_MESSAGE_TYPE:
					{
						int length = snprintf(sendBufferPtr, 256, "rm%llu %d", message.packetNumber, message.movementRequest.direction);
						
						sendBufferPtr += length + 1;
						
						if ((size_t)(sendBufferPtr - sendBuffer) >= sizeof(sendBuffer) - 256)
						{
							sendData(gNetworkConnection->socket, sendBuffer, (size_t)(sendBufferPtr - sendBuffer), &gNetworkConnection->hostAddress);
							sendBufferPtr = sendBuffer;
						}
						
						break;
					}
					case CHARACTER_FIRED_REQUEST_MESSAGE_TYPE:
					{
						int length = snprintf(sendBufferPtr, 256, "sw%llu", message.packetNumber);
						
						sendBufferPtr += length + 1;
						
						if ((size_t)(sendBufferPtr - sendBuffer) >= sizeof(sendBuffer) - 256)
						{
							sendData(gNetworkConnection->socket, sendBuffer, (size_t)(sendBufferPtr - sendBuffer), &gNetworkConnection->hostAddress);
							sendBufferPtr = sendBuffer;
						}
						
						break;
					}
					case ACK_MESSAGE_TYPE:
					{
						int length = snprintf(sendBufferPtr, 256, "ak%llu", message.packetNumber);
						
						sendBufferPtr += length + 1;
						
						if ((size_t)(sendBufferPtr - sendBuffer) >= sizeof(sendBuffer) - 256)
						{
							sendData(gNetworkConnection->socket, sendBuffer, (size_t)(sendBufferPtr - sendBuffer), &gNetworkConnection->hostAddress);
							sendBufferPtr = sendBuffer;
						}
						
						break;
					}
					case PONG_MESSAGE_TYPE:
						break;
					case COLOR_TILE_MESSAGE_TYPE:
						break;
					case TILE_FALLING_DOWN_MESSAGE_TYPE:
						break;
					case RECOVER_TILE_MESSAGE_TYPE:
						break;
					case CHARACTER_FIRED_UPDATE_MESSAGE_TYPE:
						break;
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
			
			if ((size_t)(sendBufferPtr - sendBuffer) > 0)
			{
				sendData(gNetworkConnection->socket, sendBuffer, (size_t)(sendBufferPtr - sendBuffer), &gNetworkConnection->hostAddress);
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
		
		while (!needsToQuit)
		{
			fd_set socketSet;
			FD_ZERO(&socketSet);
			FD_SET(gNetworkConnection->socket, &socketSet);
			
			struct timeval waitValue;
			waitValue.tv_sec = 0;
			waitValue.tv_usec = 0;
			
			if (select(gNetworkConnection->socket + 1, &socketSet, NULL, NULL, &waitValue) <= 0)
			{
				break;
			}
			else
			{
				int numberOfBytes = 0;
				char packetBuffer[4096];
				unsigned int addressLength = sizeof(gNetworkConnection->hostAddress);
				
				if ((numberOfBytes = recvfrom(gNetworkConnection->socket, packetBuffer, sizeof(packetBuffer), 0, (struct sockaddr *)&gNetworkConnection->hostAddress, &addressLength)) == -1)
				{
					fprintf(stderr, "recvfrom() actually returned -1\n");
				}
				else
				{
					char *buffer = packetBuffer;
					while ((int)(buffer - packetBuffer) < numberOfBytes)
					{
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
								uint64_t packetNumber = 0;
								int slotID = 0;
								int characterLives = 0;
								
								sscanf(buffer, "sr%llu %d %d", &packetNumber, &slotID, &characterLives);
								
								if (packetNumber == triggerIncomingPacketNumber + 1)
								{
									triggerIncomingPacketNumber++;
									
									GameMessage message;
									message.type = FIRST_SERVER_RESPONSE_MESSAGE_TYPE;
									message.firstServerResponse.slotID = slotID;
									message.firstServerResponse.characterLives = characterLives;
									
									pushNetworkMessage(&gGameMessagesFromNet, message);
								}
								
								if (packetNumber <= triggerIncomingPacketNumber)
								{
									GameMessage ackMessage;
									ackMessage.type = ACK_MESSAGE_TYPE;
									ackMessage.packetNumber = packetNumber;
									pushNetworkMessage(&gGameMessagesToNet, ackMessage);
								}
							}
						}
						else if (buffer[0] == 'n' && buffer[1] == 'w')
						{
							uint64_t packetNumber = 0;
							int numberOfWaitingPlayers = 0;
							sscanf(buffer + 2, "%llu %d", &packetNumber, &numberOfWaitingPlayers);
							
							if (packetNumber == triggerIncomingPacketNumber + 1 && numberOfWaitingPlayers >= 0 && numberOfWaitingPlayers < 4)
							{
								triggerIncomingPacketNumber++;
								
								GameMessage message;
								message.type = NUMBER_OF_PLAYERS_WAITING_FOR_MESSAGE_TYPE;
								message.numberOfWaitingPlayers = numberOfWaitingPlayers;
								pushNetworkMessage(&gGameMessagesFromNet, message);
							}
							
							if (packetNumber <= triggerIncomingPacketNumber)
							{
								GameMessage ackMessage;
								ackMessage.type = ACK_MESSAGE_TYPE;
								ackMessage.packetNumber = packetNumber;
								pushNetworkMessage(&gGameMessagesToNet, ackMessage);
							}
						}
						else if (buffer[0] == 'n' && buffer[1] == 'n')
						{
							// net name
							uint64_t packetNumber = 0;
							int characterID = 0;
							char *netName = malloc(MAX_USER_NAME_SIZE);
							if (netName != NULL)
							{
								memset(netName, 0, MAX_USER_NAME_SIZE);
								sscanf(buffer + 2, "%llu %d %s", &packetNumber, &characterID, netName);
								
								if (packetNumber == triggerIncomingPacketNumber + 1 && characterID > NO_CHARACTER && characterID <= PINK_BUBBLE_GUM)
								{
									triggerIncomingPacketNumber++;
									
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
								
								if (packetNumber <= triggerIncomingPacketNumber)
								{
									GameMessage ackMessage;
									ackMessage.type = ACK_MESSAGE_TYPE;
									ackMessage.packetNumber = packetNumber;
									pushNetworkMessage(&gGameMessagesToNet, ackMessage);
								}
							}
						}
						else if (buffer[0] == 's' && buffer[1] == 'g')
						{
							// start game
							uint64_t packetNumber = 0;
							sscanf(buffer + 2, "%llu", &packetNumber);
							
							if (packetNumber == triggerIncomingPacketNumber + 1)
							{
								triggerIncomingPacketNumber++;
								
								GameMessage message;
								message.type = START_GAME_MESSAGE_TYPE;
								pushNetworkMessage(&gGameMessagesFromNet, message);
							}
							
							if (packetNumber <= triggerIncomingPacketNumber)
							{
								GameMessage ackMessage;
								ackMessage.type = ACK_MESSAGE_TYPE;
								ackMessage.packetNumber = packetNumber;
								pushNetworkMessage(&gGameMessagesToNet, ackMessage);
							}
						}
						else if (buffer[0] == 'g' && buffer[1] == 's')
						{
							uint64_t packetNumber = 0;
							int gameStartNumber;
							sscanf(buffer + 2, "%llu %d", &packetNumber, &gameStartNumber);
							
							if (packetNumber == triggerIncomingPacketNumber + 1)
							{
								triggerIncomingPacketNumber++;
								
								GameMessage message;
								message.type = GAME_START_NUMBER_UPDATE_MESSAGE_TYPE;
								message.gameStartNumber = gameStartNumber;
								pushNetworkMessage(&gGameMessagesFromNet, message);
								
								GameMessage ackMessage;
								ackMessage.type = ACK_MESSAGE_TYPE;
								ackMessage.packetNumber = packetNumber;
								pushNetworkMessage(&gGameMessagesToNet, ackMessage);
							}
							
							if (packetNumber <= triggerIncomingPacketNumber)
							{
								GameMessage ackMessage;
								ackMessage.type = ACK_MESSAGE_TYPE;
								ackMessage.packetNumber = packetNumber;
								pushNetworkMessage(&gGameMessagesToNet, ackMessage);
							}
						}
						else if (buffer[0] == 'm' && buffer[1] == 'o')
						{
							uint64_t packetNumber = 0;
							int characterID = 0;
							int direction = 0;
							int pointing_direction = 0;
							uint32_t timestamp = 0;
							float x = 0.0f;
							float y = 0.0f;
							float z = 0.0f;
							
							sscanf(buffer + 2, "%llu %d %f %f %f %d %d %u", &packetNumber, &characterID, &x, &y, &z, &direction, &pointing_direction, &timestamp);
							
							if (packetNumber > realTimeIncomingPacketNumber && characterID > NO_CHARACTER && characterID <= PINK_BUBBLE_GUM)
							{
								realTimeIncomingPacketNumber = packetNumber;
								
								GameMessage message;
								message.type = CHARACTER_MOVED_UPDATE_MESSAGE_TYPE;
								message.movedUpdate.characterID = characterID;
								message.movedUpdate.x = x;
								message.movedUpdate.y = y;
								message.movedUpdate.z = z;
								message.movedUpdate.direction = direction;
								message.movedUpdate.pointing_direction = pointing_direction;
								message.movedUpdate.timestamp = timestamp;
								pushNetworkMessage(&gGameMessagesFromNet, message);
							}
						}
						else if (buffer[0] == 'p' && buffer[1] == 'k')
						{
							// character gets killed
							
							uint64_t packetNumber = 0;
							int characterID = 0;
							int characterLives = 0;
							sscanf(buffer + 2, "%llu %d %d", &packetNumber, &characterID, &characterLives);
							
							if (packetNumber == triggerIncomingPacketNumber + 1 && characterID > NO_CHARACTER && characterID <= PINK_BUBBLE_GUM)
							{
								triggerIncomingPacketNumber++;
								
								GameMessage message;
								message.type = CHARACTER_DIED_UPDATE_MESSAGE_TYPE;
								message.diedUpdate.characterID = characterID;
								message.diedUpdate.characterLives = characterLives;
								pushNetworkMessage(&gGameMessagesFromNet, message);
							}
							
							if (packetNumber <= triggerIncomingPacketNumber)
							{
								GameMessage ackMessage;
								ackMessage.type = ACK_MESSAGE_TYPE;
								ackMessage.packetNumber = packetNumber;
								pushNetworkMessage(&gGameMessagesToNet, ackMessage);
							}
						}
						else if (buffer[0] == 'c' && buffer[1] == 'k')
						{
							// character's kills increase
							uint64_t packetNumber = 0;
							int characterID = 0;
							int characterKills = 0;
							sscanf(buffer + 2, "%llu %d %d", &packetNumber, &characterID, &characterKills);
							
							if (packetNumber == triggerIncomingPacketNumber + 1 && characterID > NO_CHARACTER && characterID <= PINK_BUBBLE_GUM)
							{
								triggerIncomingPacketNumber++;
								
								GameMessage message;
								message.type = CHARACTER_KILLED_UPDATE_MESSAGE_TYPE;
								message.killedUpdate.characterID = characterID;
								message.killedUpdate.kills = characterKills;
								pushNetworkMessage(&gGameMessagesFromNet, message);
							}
							
							if (packetNumber <= triggerIncomingPacketNumber)
							{
								GameMessage ackMessage;
								ackMessage.type = ACK_MESSAGE_TYPE;
								ackMessage.packetNumber = packetNumber;
								pushNetworkMessage(&gGameMessagesToNet, ackMessage);
							}
						}
						else if (buffer[0] == 's' && buffer[1] == 'w')
						{
							// shoot weapon
							uint64_t packetNumber = 0;
							int characterID = 0;
							float x = 0.0f;
							float y = 0.0f;
							int pointing_direction = 0;
							
							sscanf(buffer + 2, "%llu %d %f %f %d", &packetNumber, &characterID, &x, &y, &pointing_direction);
							
							if (packetNumber == triggerIncomingPacketNumber + 1 && characterID > NO_CHARACTER && characterID <= PINK_BUBBLE_GUM)
							{
								triggerIncomingPacketNumber++;
								
								GameMessage message;
								message.type = CHARACTER_FIRED_UPDATE_MESSAGE_TYPE;
								message.firedUpdate.characterID = characterID;
								message.firedUpdate.x = x;
								message.firedUpdate.y = y;
								message.firedUpdate.direction = pointing_direction;
								
								pushNetworkMessage(&gGameMessagesFromNet, message);
							}
							
							if (packetNumber <= triggerIncomingPacketNumber)
							{
								GameMessage ackMessage;
								ackMessage.type = ACK_MESSAGE_TYPE;
								ackMessage.packetNumber = packetNumber;
								pushNetworkMessage(&gGameMessagesToNet, ackMessage);
							}
						}
						else if (buffer[0] == 'c' && buffer[1] == 't')
						{
							uint64_t packetNumber = 0;
							int characterID = 0;
							int tileIndex = 0;
							
							sscanf(buffer + 2, "%llu %d %d", &packetNumber, &characterID, &tileIndex);
							
							if (packetNumber == triggerIncomingPacketNumber + 1 && characterID > NO_CHARACTER && characterID <= PINK_BUBBLE_GUM && tileIndex >= 0 && tileIndex < NUMBER_OF_TILES)
							{
								triggerIncomingPacketNumber++;
								
								GameMessage message;
								message.type = COLOR_TILE_MESSAGE_TYPE;
								message.colorTile.characterID = characterID;
								message.colorTile.tileIndex = tileIndex;
								
								pushNetworkMessage(&gGameMessagesFromNet, message);
							}
							
							if (packetNumber <= triggerIncomingPacketNumber)
							{
								GameMessage ackMessage;
								ackMessage.type = ACK_MESSAGE_TYPE;
								ackMessage.packetNumber = packetNumber;
								pushNetworkMessage(&gGameMessagesToNet, ackMessage);
							}
						}
						else if (buffer[0] == 't' && buffer[1] == 'f')
						{
							uint64_t packetNumber = 0;
							int tileIndex = 0;
							SDL_bool dead = SDL_FALSE;
							
							sscanf(buffer + 2, "%llu %d %d", &packetNumber, &tileIndex, &dead);
							
							if (packetNumber == triggerIncomingPacketNumber + 1 && tileIndex >= 0 && tileIndex < NUMBER_OF_TILES)
							{
								triggerIncomingPacketNumber++;
								
								GameMessage message;
								message.type = TILE_FALLING_DOWN_MESSAGE_TYPE;
								message.fallingTile.tileIndex = tileIndex;
								message.fallingTile.dead = dead;
								
								pushNetworkMessage(&gGameMessagesFromNet, message);
							}
							
							if (packetNumber <= triggerIncomingPacketNumber)
							{
								GameMessage ackMessage;
								ackMessage.type = ACK_MESSAGE_TYPE;
								ackMessage.packetNumber = packetNumber;
								pushNetworkMessage(&gGameMessagesToNet, ackMessage);
							}
						}
						else if (buffer[0] == 'r' && buffer[1] == 't')
						{
							uint64_t packetNumber = 0;
							int tileIndex = 0;
							
							sscanf(buffer + 2, "%llu %d", &packetNumber, &tileIndex);
							
							if (packetNumber == triggerIncomingPacketNumber + 1 && tileIndex >= 0 && tileIndex < NUMBER_OF_TILES)
							{
								triggerIncomingPacketNumber++;
								
								GameMessage message;
								message.type = RECOVER_TILE_MESSAGE_TYPE;
								message.recoverTile.tileIndex = tileIndex;
								
								pushNetworkMessage(&gGameMessagesFromNet, message);
							}
							
							if (packetNumber <= triggerIncomingPacketNumber)
							{
								GameMessage ackMessage;
								ackMessage.type = ACK_MESSAGE_TYPE;
								ackMessage.packetNumber = packetNumber;
								pushNetworkMessage(&gGameMessagesToNet, ackMessage);
							}
						}
						else if (buffer[0] == 'n' && buffer[1] == 'g')
						{
							// new game
							uint64_t packetNumber = 0;
							sscanf(buffer + 2, "%llu", &packetNumber);
							
							if (packetNumber == triggerIncomingPacketNumber + 1)
							{
								triggerIncomingPacketNumber++;
								
								GameMessage message;
								message.type = GAME_RESET_MESSAGE_TYPE;
								
								pushNetworkMessage(&gGameMessagesFromNet, message);
							}
							
							if (packetNumber <= triggerIncomingPacketNumber)
							{
								GameMessage ackMessage;
								ackMessage.type = ACK_MESSAGE_TYPE;
								ackMessage.packetNumber = packetNumber;
								pushNetworkMessage(&gGameMessagesToNet, ackMessage);
							}
						}
						else if (buffer[0] == 'a' && buffer[1] == 'k')
						{
							uint64_t packetNumber = 0;
							sscanf(buffer + 2, "%llu", &packetNumber);
							
							SDL_bool foundAck = SDL_FALSE;
							uint64_t maxPacketCount = receivedAckPacketCount < receivedAckPacketsCapacity ? receivedAckPacketCount : receivedAckPacketsCapacity;
							for (uint64_t packetIndex = 0; packetIndex < maxPacketCount; packetIndex++)
							{
								if (packetNumber == receivedAckPacketNumbers[packetIndex])
								{
									foundAck = SDL_TRUE;
									break;
								}
							}
							if (!foundAck)
							{
								receivedAckPacketNumbers[receivedAckPacketCount % receivedAckPacketsCapacity] = packetNumber;
								receivedAckPacketCount++;
							}
						}
						else if (buffer[0] == 'p' && buffer[1] == 'o')
						{
							// pong message
							uint32_t timestamp = 0;
							sscanf(buffer + 2, "%u", &timestamp);
							
							GameMessage message;
							message.type = PONG_MESSAGE_TYPE;
							message.pongTimestamp = timestamp;
							pushNetworkMessage(&gGameMessagesFromNet, message);
						}
						else if (buffer[0] == 'q' && buffer[1] == 'u')
						{
							// quit
							GameMessage message;
							message.type = QUIT_MESSAGE_TYPE;
							pushNetworkMessage(&gGameMessagesFromNet, message);
							
							needsToQuit = SDL_TRUE;
							
							break;
						}
						
						buffer += strlen(buffer) + 1;
					}
				}
			}
		}
		
		if (!needsToQuit)
		{
			uint32_t timeAfterProcessing = SDL_GetTicks();
			uint32_t deltaProcessingTime = timeAfterProcessing - timeBeforeProcessing;
			const uint32_t targetDelay = 5;
			
			if (deltaProcessingTime < targetDelay)
			{
				SDL_Delay(targetDelay - deltaProcessingTime);
			}
		}
	}
	
	return 0;
}

static void initializeGameBuffer(GameMessageArray *messageArray)
{
	messageArray->messages = NULL;
	messageArray->count = 0;
	messageArray->capacity = 1024;
	messageArray->mutex = SDL_CreateMutex();
}

void initializeNetworkBuffers(void)
{
	initializeGameBuffer(&gGameMessagesFromNet);
	initializeGameBuffer(&gGameMessagesToNet);
	
	gCurrentSlotMutex = SDL_CreateMutex();
}

static void _pushNetworkMessage(GameMessageArray *messageArray, GameMessage message)
{
	SDL_bool appending = SDL_TRUE;
	
	if (messageArray->messages == NULL)
	{
		messageArray->messages = malloc(messageArray->capacity * sizeof(message));
		if (messageArray->messages == NULL)
		{
			appending = SDL_FALSE;
		}
	}
	
	if (messageArray->count >= messageArray->capacity)
	{
		messageArray->capacity = (uint32_t)(messageArray->capacity * 1.6f);
		void *newBuffer = realloc(messageArray->messages, messageArray->capacity * sizeof(message));
		if (newBuffer == NULL)
		{
			appending = SDL_FALSE;
		}
		else
		{
			messageArray->messages = newBuffer;
		}
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
	
	message->packetNumber = 0;
	
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
	message.packetNumber = 0;
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
