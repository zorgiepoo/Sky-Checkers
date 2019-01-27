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

#include <inttypes.h>

#define NET_MESSAGE_TAG_SIZE 2

NetworkConnection *gNetworkConnection = NULL;

static SDL_mutex *gCurrentSlotMutex;

static void pushNetworkMessage(GameMessageArray *messageArray, GameMessage message);
static void depleteNetworkMessages(GameMessageArray *messageArray);

static void cleanupStateFromNetwork(void);

static void clearPredictedColors(int characterID)
{
	for (int tileIndex = 0; tileIndex < NUMBER_OF_TILES; tileIndex++)
	{
		if (gTiles[tileIndex].predictedColorID == characterID)
		{
			if (gTiles[tileIndex].coloredID == NO_CHARACTER)
			{
				restoreDefaultTileColor(tileIndex);
			}
			gTiles[tileIndex].predictedColorID = NO_CHARACTER;
		}
	}
}

void setPredictedDirection(Character *character, int direction)
{
	character->predictedDirection = direction;
	character->predictedDirectionTime = SDL_GetTicks() + gNetworkConnection->serverHalfPing;
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
		if (character->predictedDirectionTime > 0)
		{
			uint32_t predictionTimeWithErrorMargin = character->predictedDirectionTime + gNetworkConnection->serverHalfPing * 3;
			
			if (predictionTimeWithErrorMargin >= previousMovement->ticks)
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
				
				if (predictionTimeWithErrorMargin < nextMovement->ticks)
				{
					character->predictedDirectionTime = 0;
				}
				
				checkDiscrepancy = SDL_FALSE;
			}
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
				
				if (character->movementConsumedCounter >= 2)
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
					
					deinitializeNetwork();
					
					SDL_Thread *thread = gNetworkConnection->thread;
					
					free(gNetworkConnection->characterTriggerMessages);
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
					uint8_t direction = message.movementRequest.direction;
					uint8_t characterID = message.addressIndex + 1;
					
					Character *character = getCharacter(characterID);
					character->direction = direction;
					turnCharacter(character, direction);
					
					break;
				}
				case CHARACTER_FIRED_REQUEST_MESSAGE_TYPE:
				{
					uint8_t characterID = message.firedUpdate.characterID;
					Character *character = getCharacter(characterID);
					
					uint32_t halfPing = gNetworkConnection->clientHalfPings[message.addressIndex];
					
					float compensation = (halfPing > 110 ? 110.0f : (float)halfPing) / 1000.0f;
					
					prepareFiringCharacterWeapon(character, character->x, character->y, compensation);
					
					break;
				}
				case CHARACTER_DIED_UPDATE_MESSAGE_TYPE:
				case CHARACTER_FIRED_UPDATE_MESSAGE_TYPE:
				case TILE_FALLING_DOWN_MESSAGE_TYPE:
				case RECOVER_TILE_MESSAGE_TYPE:
				case COLOR_TILE_MESSAGE_TYPE:
				{
					if (!gGameShouldReset && gNetworkConnection != NULL)
					{
						if (gNetworkConnection->characterTriggerMessages == NULL)
						{
							gNetworkConnection->characterTriggerMessagesCapacity = 64;
							gNetworkConnection->characterTriggerMessagesCount = 0;
							
							gNetworkConnection->characterTriggerMessages = malloc(sizeof(*gNetworkConnection->characterTriggerMessages) * gNetworkConnection->characterTriggerMessagesCapacity);
						}
						
						message.ticks = SDL_GetTicks() - gNetworkConnection->serverHalfPing;
						
						SDL_bool foundReusableMessage = SDL_FALSE;
						for (uint32_t messageIndex = 0; messageIndex < gNetworkConnection->characterTriggerMessagesCount; messageIndex++)
						{
							if (gNetworkConnection->characterTriggerMessages[messageIndex].ticks == 0)
							{
								// Found a message we can reuse
								gNetworkConnection->characterTriggerMessages[messageIndex] = message;
								foundReusableMessage = SDL_TRUE;
								break;
							}
						}
						
						if (!foundReusableMessage)
						{
							if (gNetworkConnection->characterTriggerMessagesCount >= gNetworkConnection->characterTriggerMessagesCapacity)
							{
								gNetworkConnection->characterTriggerMessagesCapacity = (uint32_t)(gNetworkConnection->characterTriggerMessagesCapacity * 1.6f);
								gNetworkConnection->characterTriggerMessages = realloc(gNetworkConnection->characterTriggerMessages, gNetworkConnection->characterTriggerMessagesCapacity * sizeof(*gNetworkConnection->characterTriggerMessages));
							}
							
							gNetworkConnection->characterTriggerMessages[gNetworkConnection->characterTriggerMessagesCount] = message;
							
							gNetworkConnection->characterTriggerMessagesCount++;
						}
					}
					
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
				case PONG_MESSAGE_TYPE:
				{
					uint32_t currentTicks = SDL_GetTicks();
					uint32_t halfPingTime = (uint32_t)((currentTicks - message.pongTimestamp) / 2.0f);
					
					if (gNetworkConnection->type == NETWORK_CLIENT_TYPE)
					{
						const size_t capacity = sizeof(gNetworkConnection->recentServerHalfPings) / sizeof(*gNetworkConnection->recentServerHalfPings);
						
						gNetworkConnection->recentServerHalfPings[gNetworkConnection->recentServerHalfPingIndex % capacity] = halfPingTime;
						
						gNetworkConnection->recentServerHalfPingIndex++;
						
						gNetworkConnection->serverHalfPing = 0;
						uint32_t count = 0;
						
						for (uint32_t index = 0; index < capacity; index++)
						{
							if (gNetworkConnection->recentServerHalfPings[index] != 0)
							{
								gNetworkConnection->serverHalfPing += gNetworkConnection->recentServerHalfPings[index];
								count++;
							}
						}
						
						if (count > 0)
						{
							gNetworkConnection->serverHalfPing /= count;
						}
						else
						{
							gNetworkConnection->serverHalfPing = 0;
						}
					}
					else if (gNetworkConnection->type == NETWORK_SERVER_TYPE)
					{
						const size_t capacity = sizeof(gNetworkConnection->recentClientHalfPings[0]) / sizeof(*gNetworkConnection->recentClientHalfPings[0]);
						
						gNetworkConnection->recentClientHalfPings[message.addressIndex][gNetworkConnection->recentClientHalfPingIndices[message.addressIndex] % capacity] = halfPingTime;
						
						gNetworkConnection->recentClientHalfPingIndices[message.addressIndex]++;
						
						gNetworkConnection->clientHalfPings[message.addressIndex] = 0;
						uint32_t count = 0;
						
						for (uint32_t index = 0; index < capacity; index++)
						{
							if (gNetworkConnection->recentClientHalfPings[message.addressIndex][index] != 0)
							{
								gNetworkConnection->clientHalfPings[message.addressIndex] += gNetworkConnection->recentClientHalfPings[message.addressIndex][index];
								count++;
							}
						}
						
						if (count > 0)
						{
							gNetworkConnection->clientHalfPings[message.addressIndex] /= count;
						}
						else
						{
							gNetworkConnection->clientHalfPings[message.addressIndex] = 0;
						}

					}
					
					break;
				}
				case CHARACTER_MOVED_UPDATE_MESSAGE_TYPE:
				{
					uint32_t currentTicks = SDL_GetTicks();
					
					if (!gGameShouldReset && currentTicks >= gNetworkConnection->serverHalfPing)
					{
						SDL_bool shouldSetCharacterPosition = SDL_FALSE;
						if (gNetworkConnection->serverHalfPing > 0)
						{
							CharacterMovement newMovement;
							newMovement.x = message.movedUpdate.x;
							newMovement.y = message.movedUpdate.y;
							newMovement.z = message.movedUpdate.z;
							newMovement.direction = message.movedUpdate.direction;
							newMovement.pointing_direction = message.movedUpdate.pointing_direction;
							newMovement.ticks = currentTicks - gNetworkConnection->serverHalfPing;
							
							uint8_t characterIndex = message.movedUpdate.characterID - 1;
							
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
					
					gNetworkConnection->characterTriggerMessagesCount = 0;
					
					break;
				case FIRST_SERVER_RESPONSE_MESSAGE_TYPE:
				{
					uint8_t slotID = message.firstServerResponse.slotID;
					uint8_t characterLives = message.firstServerResponse.characterLives;
					
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
					
					for (uint8_t characterIndex = RED_ROVER; characterIndex <= PINK_BUBBLE_GUM; characterIndex++)
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
			uint32_t renderTime = currentTime - (uint32_t)(3 * gNetworkConnection->serverHalfPing);
			
			SDL_bool clearedPredictedColors = SDL_FALSE;
			for (uint32_t triggerMessageIndex = 0; triggerMessageIndex < gNetworkConnection->characterTriggerMessagesCount; triggerMessageIndex++)
			{
				GameMessage *message = &gNetworkConnection->characterTriggerMessages[triggerMessageIndex];
				if (message->ticks != 0 && renderTime >= message->ticks)
				{
					if (message->type == CHARACTER_FIRED_UPDATE_MESSAGE_TYPE)
					{
						uint8_t characterID = message->firedUpdate.characterID;
						Character *character = getCharacter(characterID);
						
						character->pointing_direction = message->firedUpdate.direction;
						prepareFiringCharacterWeapon(character, message->firedUpdate.x, message->firedUpdate.y, 0.0f);
					}
					else if (message->type == COLOR_TILE_MESSAGE_TYPE)
					{
						uint8_t characterID = message->colorTile.characterID;
						uint8_t tileIndex = message->colorTile.tileIndex;
						Character *character = getCharacter(characterID);
						
						if (!clearedPredictedColors && character == gNetworkConnection->character)
						{
							clearPredictedColors(characterID);
							clearedPredictedColors = SDL_TRUE;
						}
						
						gTiles[tileIndex].red = character->weap->red;
						gTiles[tileIndex].green = character->weap->green;
						gTiles[tileIndex].blue = character->weap->blue;
						gTiles[tileIndex].coloredID = characterID;
					}
					else if (message->type == TILE_FALLING_DOWN_MESSAGE_TYPE)
					{
						uint8_t tileIndex = message->fallingTile.tileIndex;
						if (message->fallingTile.dead)
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
					}
					else if (message->type == RECOVER_TILE_MESSAGE_TYPE)
					{
						recoverDestroyedTile(message->recoverTile.tileIndex);
					}
					else if (message->type == CHARACTER_DIED_UPDATE_MESSAGE_TYPE)
					{
						Character *character = getCharacter(message->diedUpdate.characterID);
						character->lives = message->diedUpdate.characterLives;
						
						prepareCharactersDeath(character);
						clearPredictedColors(message->diedUpdate.characterID);
						
						decideWhetherToMakeAPlayerAWinner(character);
						
						character->z -= OBJECT_FALLING_STEP;
					}
					
					// Mark message as already visited
					message->ticks = 0;
				}
			}
			
			for (uint8_t characterID = RED_ROVER; characterID <= PINK_BUBBLE_GUM; characterID++)
			{
				uint8_t characterIndex = characterID - 1;

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
		for (uint8_t characterID = RED_ROVER; characterID <= PINK_BUBBLE_GUM; characterID++)
		{
			Character *character = getCharacter(characterID);
			float displacementAdjustment = timeDelta * INITIAL_CHARACTER_SPEED / 32.0f;
			
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

static void sendData(int socket, void *data, size_t size, SocketAddress *address)
{
	// Don't use sa_len to get the size because it could not be portable
	if (address->sa.sa_family == AF_INET)
	{
		sendto(socket, data, size, 0, &address->sa, sizeof(address->sa_in));
	}
	else if (address->sa.sa_family == AF_INET6)
	{
		sendto(socket, data, size, 0, &address->sa, sizeof(address->sa_in6));
	}
}

#ifdef WINDOWS
static int
#else
static ssize_t
#endif
receiveData(int socket, void *buffer, size_t length, SocketAddress *address)
{
	memset(address, 0, sizeof(*address));
	socklen_t addressLength = sizeof(*address);
	return recvfrom(socket, buffer, length, 0, &address->sa, &addressLength);
}

static uint8_t characterIDForClientAddress(SocketAddress *address)
{
	for (uint8_t clientIndex = 0; clientIndex < gCurrentSlot; clientIndex++)
	{
		if (memcmp(address, &gNetworkConnection->clientAddresses[clientIndex], sizeof(*address)) == 0)
		{
			return clientIndex + 1;
		}
	}
	return NO_CHARACTER;
}

static void advanceSendBuffer(char **sendBufferPtr, const void *data, size_t size)
{
	memcpy(*sendBufferPtr, data, size);
	*sendBufferPtr += size;
}

#define ADVANCE_SEND_BUFFER(sendBufferPtr, data) advanceSendBuffer((sendBufferPtr), &(data), sizeof(data))

static void advanceSendBufferForInitialMessage(char **sendBufferPtr, const char *tag, uint32_t packetNumber)
{
	advanceSendBuffer(sendBufferPtr, tag, NET_MESSAGE_TAG_SIZE);
	ADVANCE_SEND_BUFFER(sendBufferPtr, packetNumber);
}

static void sendAndResetBufferIfNeeded(char *sendBuffer, size_t sendBufferSize, char **sendBufferPtr, SocketAddress *address)
{
	if ((size_t)(*sendBufferPtr - sendBuffer) >= sendBufferSize - 256)
	{
		sendData(gNetworkConnection->socket, sendBuffer, (size_t)(*sendBufferPtr - sendBuffer), address);
		*sendBufferPtr = sendBuffer;
	}
}

static void advanceReceiveBuffer(char **buffer, void *receiveData, size_t receiveDataSize)
{
	memcpy(receiveData, *buffer, receiveDataSize);
	*buffer += receiveDataSize;
}

#define ADVANCE_RECEIVE_BUFFER(buffer, data) advanceReceiveBuffer(buffer, &(data), sizeof((data)))

int serverNetworkThread(void *initialNumberOfPlayersToWaitForPtr)
{
	gCurrentSlot = 0;
	uint8_t numberOfPlayersToWaitFor = *(uint8_t *)initialNumberOfPlayersToWaitForPtr;
	
	uint32_t triggerOutgoingPacketNumbers[] = {1, 1, 1};
	uint32_t realTimeOutgoingPacketNumbers[] = {1, 1, 1};
	
	uint32_t triggerIncomingPacketNumbers[] = {0, 0, 0};
	
	uint32_t receivedAckPacketNumbers[3][256] = {{0}, {0}, {0}};
	size_t receivedAckPacketsCapacity = sizeof(receivedAckPacketNumbers[0]) / sizeof(*receivedAckPacketNumbers[0]);
	uint32_t receivedAckPacketCount = 0;
	
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
			// Only keep one ping message per character per packet
			uint32_t trackedPingIndices[3] = {0, 0, 0};
			for (uint32_t messagesLeft = messagesCount; messagesLeft > 0; messagesLeft--)
			{
				GameMessage message = messagesAvailable[messagesLeft - 1];
				if (message.type == CHARACTER_MOVED_UPDATE_MESSAGE_TYPE)
				{
					int addressIndex = message.addressIndex;
					uint8_t characterIndex = message.movedUpdate.characterID - 1;
					
					if (trackedMovementIndices[addressIndex][characterIndex] == 0)
					{
						trackedMovementIndices[addressIndex][characterIndex] = messagesLeft - 1;
					}
					else
					{
						messagesAvailable[messagesLeft - 1].addressIndex = -1;
					}
				}
				else if (message.type == PING_MESSAGE_TYPE)
				{
					int addressIndex = message.addressIndex;
					
					if (trackedPingIndices[addressIndex] == 0)
					{
						trackedPingIndices[addressIndex] = messagesLeft - 1;
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
				SocketAddress *address = (addressIndex == -1) ? NULL : &gNetworkConnection->clientAddresses[addressIndex];
				
				if (!needsToQuit && message.type != QUIT_MESSAGE_TYPE && message.type != ACK_MESSAGE_TYPE && message.type != FIRST_DATA_TO_CLIENT_MESSAGE_TYPE && message.type != PING_MESSAGE_TYPE && message.type != PONG_MESSAGE_TYPE)
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
						uint32_t maxPacketCount = receivedAckPacketCount < receivedAckPacketsCapacity ? receivedAckPacketCount : receivedAckPacketsCapacity;
						for (uint32_t packetIndex = 0; packetIndex < maxPacketCount; packetIndex++)
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
					case QUIT_MESSAGE_TYPE:
					{
						if (address != NULL)
						{
							sendData(gNetworkConnection->socket, "qu", NET_MESSAGE_TAG_SIZE, address);
						}
						
						needsToQuit = SDL_TRUE;
						
						break;
					}
					case MOVEMENT_REQUEST_MESSAGE_TYPE:
						break;
					case CHARACTER_FIRED_UPDATE_MESSAGE_TYPE:
					{
						advanceSendBufferForInitialMessage(&sendBufferPtrs[addressIndex], "sw", message.packetNumber);
						
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.firedUpdate.characterID);
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.firedUpdate.x);
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.firedUpdate.y);
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.firedUpdate.direction);
						
						sendAndResetBufferIfNeeded(sendBuffers[addressIndex], sizeof(sendBuffers[addressIndex]), &sendBufferPtrs[addressIndex], address);
						
						break;
					}
					case CHARACTER_FIRED_REQUEST_MESSAGE_TYPE:
						break;
					case NUMBER_OF_PLAYERS_WAITING_FOR_MESSAGE_TYPE:
					{
						advanceSendBufferForInitialMessage(&sendBufferPtrs[addressIndex], "nw", message.packetNumber);
						
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.numberOfWaitingPlayers);
						
						sendAndResetBufferIfNeeded(sendBuffers[addressIndex], sizeof(sendBuffers[addressIndex]), &sendBufferPtrs[addressIndex], address);
						
						break;
					}
					case NET_NAME_MESSAGE_TYPE:
					{
						advanceSendBufferForInitialMessage(&sendBufferPtrs[addressIndex], "nn", message.packetNumber);
						
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.netNameRequest.characterID);
						
						char netName[MAX_USER_NAME_SIZE] = {0};
						strncpy(netName, message.netNameRequest.netName, MAX_USER_NAME_SIZE - 1);
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], netName);
						
						sendAndResetBufferIfNeeded(sendBuffers[addressIndex], sizeof(sendBuffers[addressIndex]), &sendBufferPtrs[addressIndex], address);
						
						break;
					}
					case START_GAME_MESSAGE_TYPE:
					{
						advanceSendBufferForInitialMessage(&sendBufferPtrs[addressIndex], "sg", message.packetNumber);
						
						sendAndResetBufferIfNeeded(sendBuffers[addressIndex], sizeof(sendBuffers[addressIndex]), &sendBufferPtrs[addressIndex], address);
						
						break;
					}
					case GAME_START_NUMBER_UPDATE_MESSAGE_TYPE:
					{
						advanceSendBufferForInitialMessage(&sendBufferPtrs[addressIndex], "gs", message.packetNumber);
						
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.gameStartNumber);
						
						sendAndResetBufferIfNeeded(sendBuffers[addressIndex], sizeof(sendBuffers[addressIndex]), &sendBufferPtrs[addressIndex], address);
						
						break;
					}
					case CHARACTER_DIED_UPDATE_MESSAGE_TYPE:
					{
						advanceSendBufferForInitialMessage(&sendBufferPtrs[addressIndex], "pk", message.packetNumber);
						
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.diedUpdate.characterID);
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.diedUpdate.characterLives);
						
						sendAndResetBufferIfNeeded(sendBuffers[addressIndex], sizeof(sendBuffers[addressIndex]), &sendBufferPtrs[addressIndex], address);
						
						break;
					}
					case COLOR_TILE_MESSAGE_TYPE:
					{
						advanceSendBufferForInitialMessage(&sendBufferPtrs[addressIndex], "ct", message.packetNumber);
						
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.colorTile.characterID);
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.colorTile.tileIndex);
						
						sendAndResetBufferIfNeeded(sendBuffers[addressIndex], sizeof(sendBuffers[addressIndex]), &sendBufferPtrs[addressIndex], address);
						
						break;
					}
					case TILE_FALLING_DOWN_MESSAGE_TYPE:
					{
						advanceSendBufferForInitialMessage(&sendBufferPtrs[addressIndex], "tf", message.packetNumber);
						
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.fallingTile.tileIndex);
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.fallingTile.dead);
						
						sendAndResetBufferIfNeeded(sendBuffers[addressIndex], sizeof(sendBuffers[addressIndex]), &sendBufferPtrs[addressIndex], address);
						
						break;
					}
					case RECOVER_TILE_MESSAGE_TYPE:
					{
						advanceSendBufferForInitialMessage(&sendBufferPtrs[addressIndex], "rt", message.packetNumber);
						
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.recoverTile.tileIndex);
						
						sendAndResetBufferIfNeeded(sendBuffers[addressIndex], sizeof(sendBuffers[addressIndex]), &sendBufferPtrs[addressIndex], address);
						
						break;
					}
					case CHARACTER_MOVED_UPDATE_MESSAGE_TYPE:
					{
						advanceSendBufferForInitialMessage(&sendBufferPtrs[addressIndex], "mo", message.packetNumber);
						
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.movedUpdate.characterID);
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.movedUpdate.x);
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.movedUpdate.y);
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.movedUpdate.z);
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.movedUpdate.direction);
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.movedUpdate.pointing_direction);
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.movedUpdate.timestamp);
						
						sendAndResetBufferIfNeeded(sendBuffers[addressIndex], sizeof(sendBuffers[addressIndex]), &sendBufferPtrs[addressIndex], address);
						
						break;
					}
					case CHARACTER_KILLED_UPDATE_MESSAGE_TYPE:
					{
						advanceSendBufferForInitialMessage(&sendBufferPtrs[addressIndex], "ck", message.packetNumber);
						
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.killedUpdate.characterID);
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.killedUpdate.kills);
						
						sendAndResetBufferIfNeeded(sendBuffers[addressIndex], sizeof(sendBuffers[addressIndex]), &sendBufferPtrs[addressIndex], address);
						
						break;
					}
					case GAME_RESET_MESSAGE_TYPE:
					{
						advanceSendBufferForInitialMessage(&sendBufferPtrs[addressIndex], "ng", message.packetNumber);
						
						sendAndResetBufferIfNeeded(sendBuffers[addressIndex], sizeof(sendBuffers[addressIndex]), &sendBufferPtrs[addressIndex], address);
						
						break;
					}
					case ACK_MESSAGE_TYPE:
					{
						advanceSendBufferForInitialMessage(&sendBufferPtrs[addressIndex], "ak", message.packetNumber);
						
						sendAndResetBufferIfNeeded(sendBuffers[addressIndex], sizeof(sendBuffers[addressIndex]), &sendBufferPtrs[addressIndex], address);
						
						break;
					}
					case PING_MESSAGE_TYPE:
					{
						if (address != NULL)
						{
							advanceSendBuffer(&sendBufferPtrs[addressIndex], "pi", NET_MESSAGE_TAG_SIZE);
							
							ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.pingTimestamp);
							
							sendAndResetBufferIfNeeded(sendBuffers[addressIndex], sizeof(sendBuffers[addressIndex]), &sendBufferPtrs[addressIndex], address);
						}
						
						break;
					}
					case PONG_MESSAGE_TYPE:
					{
						advanceSendBuffer(&sendBufferPtrs[addressIndex], "po", NET_MESSAGE_TAG_SIZE);
						
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.pongTimestamp);
						
						sendAndResetBufferIfNeeded(sendBuffers[addressIndex], sizeof(sendBuffers[addressIndex]), &sendBufferPtrs[addressIndex], address);
						
						break;
					}
					case FIRST_SERVER_RESPONSE_MESSAGE_TYPE:
					{
						advanceSendBufferForInitialMessage(&sendBufferPtrs[addressIndex], "sr", message.packetNumber);
						
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.firstServerResponse.slotID);
						ADVANCE_SEND_BUFFER(&sendBufferPtrs[addressIndex], message.firstServerResponse.characterLives);
						
						sendAndResetBufferIfNeeded(sendBuffers[addressIndex], sizeof(sendBuffers[addressIndex]), &sendBufferPtrs[addressIndex], address);
						
						break;
					}
					case FIRST_CLIENT_RESPONSE_MESSAGE_TYPE:
						break;
					case FIRST_DATA_TO_CLIENT_MESSAGE_TYPE:
					{
						uint8_t clientCharacterID = message.firstDataToClient.characterID;
						
						// send client its initial info
						{
							GameMessage responseMessage;
							responseMessage.type = FIRST_SERVER_RESPONSE_MESSAGE_TYPE;
							responseMessage.packetNumber = 0;
							responseMessage.firstServerResponse.slotID = clientCharacterID - 1;
							responseMessage.firstServerResponse.characterLives = gCharacterNetLives;
							
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
						
						for (uint8_t characterIndex = RED_ROVER; characterIndex <= PINK_BUBBLE_GUM; characterIndex++)
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
						
						uint8_t numPlayersToWaitFor = message.firstDataToClient.numberOfPlayersToWaitFor;
						if (numPlayersToWaitFor == 0)
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
					sendData(gNetworkConnection->socket, sendBuffers[addressIndex], (size_t)(sendBufferPtrs[addressIndex] - sendBuffers[addressIndex]), &gNetworkConnection->clientAddresses[addressIndex]);
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
				SocketAddress address;
				int numberOfBytes;
				if ((numberOfBytes = receiveData(gNetworkConnection->socket, packetBuffer, sizeof(packetBuffer), &address)) == -1)
				{
					// Ignore it and continue on
					fprintf(stderr, "receiveData() actually returned -1\n");
				}
				else
				{
					char *buffer = packetBuffer;
					while (buffer + NET_MESSAGE_TAG_SIZE <= packetBuffer + numberOfBytes)
					{
						uint8_t messageTag[NET_MESSAGE_TAG_SIZE] = {0};
						ADVANCE_RECEIVE_BUFFER(&buffer, messageTag);
						
						// can i play?
						if (messageTag[0] == 'c' && messageTag[1] == 'p')
						{
							uint32_t packetNumber = 0;
							
							if (buffer + sizeof(packetNumber) + MAX_USER_NAME_SIZE <= packetBuffer + numberOfBytes)
							{
								ADVANCE_RECEIVE_BUFFER(&buffer, packetNumber);
								
								char *netName = calloc(MAX_USER_NAME_SIZE, 1);
								strncpy(netName, buffer, MAX_USER_NAME_SIZE - 1);
								buffer += MAX_USER_NAME_SIZE;
								
								uint8_t existingCharacterID = characterIDForClientAddress(&address);
								if ((packetNumber == 1 && existingCharacterID == NO_CHARACTER && numberOfPlayersToWaitFor > 0) || (packetNumber == 1 && existingCharacterID != NO_CHARACTER))
								{
									int addressIndex;
									if (existingCharacterID == NO_CHARACTER)
									{
										// yes
										// sr == server response
										addressIndex = gCurrentSlot;
										gNetworkConnection->clientAddresses[addressIndex] = address;
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
									// sn == server no rejection response
									sendData(gNetworkConnection->socket, "sn", NET_MESSAGE_TAG_SIZE, &address);
								}
							}
						}
						
						else if (messageTag[0] == 'r' && messageTag[1] == 'm')
						{
							// request movement
							uint8_t characterID = characterIDForClientAddress(&address);
							if (characterID > NO_CHARACTER && characterID <= PINK_BUBBLE_GUM)
							{
								uint8_t direction = 0;
								uint32_t packetNumber = 0;
								if (buffer + sizeof(packetNumber) + sizeof(direction) <= packetBuffer + numberOfBytes)
								{
									uint8_t addressIndex = characterID - 1;
									
									ADVANCE_RECEIVE_BUFFER(&buffer, packetNumber);
									ADVANCE_RECEIVE_BUFFER(&buffer, direction);
									
									GameMessage message;
									message.packetNumber = packetNumber;
									message.type = MOVEMENT_REQUEST_MESSAGE_TYPE;
									message.addressIndex = addressIndex;
									message.movementRequest.direction = direction;
									
									if (packetNumber == triggerIncomingPacketNumbers[addressIndex] + 1)
									{
										triggerIncomingPacketNumbers[addressIndex]++;
										
										if (direction == LEFT || direction == RIGHT || direction == UP || direction == DOWN || direction == NO_DIRECTION)
										{
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
							}
						}
						
						else if (messageTag[0] == 's' && messageTag[1] == 'w')
						{
							// shoot weapon
							uint32_t packetNumber = 0;
							if (buffer + sizeof(packetNumber) <= packetBuffer + numberOfBytes)
							{
								ADVANCE_RECEIVE_BUFFER(&buffer, packetNumber);
								
								uint8_t characterID = characterIDForClientAddress(&address);
								if (characterID > NO_CHARACTER && characterID <= PINK_BUBBLE_GUM)
								{
									uint8_t addressIndex = characterID - 1;
									
									GameMessage message;
									message.packetNumber = packetNumber;
									message.type = CHARACTER_FIRED_REQUEST_MESSAGE_TYPE;
									message.addressIndex = addressIndex;
									message.firedRequest.characterID = characterID;
									
									if (packetNumber == triggerIncomingPacketNumbers[addressIndex] + 1)
									{
										triggerIncomingPacketNumbers[addressIndex]++;
										
										pushNetworkMessage(&gGameMessagesFromNet, message);
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
							}
						}
						
						else if (messageTag[0] == 'a' && messageTag[1] == 'k')
						{
							uint32_t packetNumber = 0;
							if (buffer + sizeof(packetNumber) <= packetBuffer + numberOfBytes)
							{
								ADVANCE_RECEIVE_BUFFER(&buffer, packetNumber);
								
								uint8_t characterID = characterIDForClientAddress(&address);
								if (characterID != NO_CHARACTER)
								{
									uint8_t addressIndex = characterID - 1;
									
									SDL_bool foundAck = SDL_FALSE;
									uint32_t maxPacketCount = receivedAckPacketCount < receivedAckPacketsCapacity ? receivedAckPacketCount : receivedAckPacketsCapacity;
									for (uint32_t packetIndex = 0; packetIndex < maxPacketCount; packetIndex++)
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
							}
						}
						
						else if (messageTag[0] == 'p' && messageTag[1] == 'i')
						{
							uint32_t timestamp = 0;
							if (buffer + sizeof(timestamp) <= packetBuffer + numberOfBytes)
							{
								ADVANCE_RECEIVE_BUFFER(&buffer, timestamp);
								
								uint8_t characterID = characterIDForClientAddress(&address);
								if (characterID != NO_CHARACTER)
								{
									uint8_t addressIndex = characterID - 1;
									
									GameMessage pongMessage;
									pongMessage.type = PONG_MESSAGE_TYPE;
									pongMessage.addressIndex = addressIndex;
									pongMessage.pongTimestamp = timestamp;
									pushNetworkMessage(&gGameMessagesToNet, pongMessage);
								}
							}
						}
						
						else if (messageTag[0] == 'p' && messageTag[1] == 'o')
						{
							// pong message
							uint32_t timestamp = 0;
							if (buffer + sizeof(timestamp) <= packetBuffer + numberOfBytes)
							{
								ADVANCE_RECEIVE_BUFFER(&buffer, timestamp);
								
								uint8_t characterID = characterIDForClientAddress(&address);
								if (characterID != NO_CHARACTER)
								{
									uint8_t addressIndex = characterID - 1;
									
									GameMessage message;
									message.type = PONG_MESSAGE_TYPE;
									message.addressIndex = addressIndex;
									message.pongTimestamp = timestamp;
									pushNetworkMessage(&gGameMessagesFromNet, message);
								}
							}
						}
						
						else if (messageTag[0] == 'q' && messageTag[1] == 'u')
						{
							GameMessage message;
							message.type = QUIT_MESSAGE_TYPE;
							
							uint8_t characterID = characterIDForClientAddress(&address);
							
							if (characterID != NO_CHARACTER)
							{
								sendToClients(characterID, &message);
							}
						}
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
	
	deinitializeNetwork();
	
	return 0;
}

int clientNetworkThread(void *context)
{
	uint32_t triggerOutgoingPacketNumber = 1;
	
	uint32_t triggerIncomingPacketNumber = 0;
	uint32_t realTimeIncomingPacketNumber = 0;
	
	uint32_t receivedAckPacketNumbers[256] = {0};
	size_t receivedAckPacketsCapacity = sizeof(receivedAckPacketNumbers) / sizeof(*receivedAckPacketNumbers);
	uint32_t receivedAckPacketCount = 0;
	
	// tell the server we exist
	GameMessage welcomeMessage;
	welcomeMessage.type = WELCOME_MESSAGE_TO_SERVER_MESSAGE_TYPE;
	welcomeMessage.weclomeMessage.netName = gUserNameString;
	sendToServer(welcomeMessage);
	
	uint32_t lastPongReceivedTimestamp = SDL_GetTicks();
	
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
			
			uint32_t lastPingIndex = 0;
			for (uint32_t messagesLeft = messagesCount; messagesLeft > 0; messagesLeft--)
			{
				GameMessage message = messagesAvailable[messagesLeft - 1];
				if (message.type == PING_MESSAGE_TYPE)
				{
					if (lastPingIndex == 0)
					{
						lastPingIndex = messagesLeft - 1;
						break;
					}
				}
			}
			
			for (uint32_t messageIndex = 0; messageIndex < messagesCount && !needsToQuit; messageIndex++)
			{
				GameMessage message = messagesAvailable[messageIndex];
				if (message.type != QUIT_MESSAGE_TYPE && message.type != ACK_MESSAGE_TYPE && message.type != PING_MESSAGE_TYPE && message.type != PONG_MESSAGE_TYPE)
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
						uint32_t maxPacketCount = receivedAckPacketCount < receivedAckPacketsCapacity ? receivedAckPacketCount + 1 : receivedAckPacketsCapacity;
						for (uint32_t packetIndex = 0; packetIndex < maxPacketCount; packetIndex++)
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
						advanceSendBufferForInitialMessage(&sendBufferPtr, "cp", message.packetNumber);
						
						ADVANCE_SEND_BUFFER(&sendBufferPtr, gUserNameString);
						
						sendAndResetBufferIfNeeded(sendBuffer, sizeof(sendBuffer), &sendBufferPtr, &gNetworkConnection->hostAddress);
						
						break;
					}
					case QUIT_MESSAGE_TYPE:
					{
						sendData(gNetworkConnection->socket, "qu", NET_MESSAGE_TAG_SIZE, &gNetworkConnection->hostAddress);
						
						needsToQuit = SDL_TRUE;
						
						break;
					}
					case PING_MESSAGE_TYPE:
					{
						if (lastPingIndex == messageIndex)
						{
							advanceSendBuffer(&sendBufferPtr, "pi", NET_MESSAGE_TAG_SIZE);
							
							ADVANCE_SEND_BUFFER(&sendBufferPtr, message.pingTimestamp);
							
							sendAndResetBufferIfNeeded(sendBuffer, sizeof(sendBuffer), &sendBufferPtr, &gNetworkConnection->hostAddress);
						}
						
						break;
					}
					case PONG_MESSAGE_TYPE:
					{
						advanceSendBuffer(&sendBufferPtr, "po", NET_MESSAGE_TAG_SIZE);
						
						ADVANCE_SEND_BUFFER(&sendBufferPtr, message.pongTimestamp);
						
						sendAndResetBufferIfNeeded(sendBuffer, sizeof(sendBuffer), &sendBufferPtr, &gNetworkConnection->hostAddress);
						
						break;
					}
					case MOVEMENT_REQUEST_MESSAGE_TYPE:
					{
						advanceSendBufferForInitialMessage(&sendBufferPtr, "rm", message.packetNumber);
						
						ADVANCE_SEND_BUFFER(&sendBufferPtr, message.movementRequest.direction);
						
						sendAndResetBufferIfNeeded(sendBuffer, sizeof(sendBuffer), &sendBufferPtr, &gNetworkConnection->hostAddress);
						
						break;
					}
					case CHARACTER_FIRED_REQUEST_MESSAGE_TYPE:
					{
						advanceSendBufferForInitialMessage(&sendBufferPtr, "sw", message.packetNumber);
						
						sendAndResetBufferIfNeeded(sendBuffer, sizeof(sendBuffer), &sendBufferPtr, &gNetworkConnection->hostAddress);
						
						break;
					}
					case ACK_MESSAGE_TYPE:
					{
						advanceSendBufferForInitialMessage(&sendBufferPtr, "ak", message.packetNumber);
						
						sendAndResetBufferIfNeeded(sendBuffer, sizeof(sendBuffer), &sendBufferPtr, &gNetworkConnection->hostAddress);
						
						break;
					}
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
		
		// 4 seconds is a long time without hearing back from server
		if (SDL_GetTicks() - lastPongReceivedTimestamp >= 4000)
		{
			GameMessage message;
			message.type = QUIT_MESSAGE_TYPE;
			pushNetworkMessage(&gGameMessagesFromNet, message);
			
			needsToQuit = SDL_TRUE;
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
				int numberOfBytes;
				if ((numberOfBytes = receiveData(gNetworkConnection->socket, packetBuffer, sizeof(packetBuffer), &gNetworkConnection->hostAddress)) == -1)
				{
					fprintf(stderr, "receiveData() actually returned -1\n");
				}
				else
				{
					char *buffer = packetBuffer;
					while (buffer + NET_MESSAGE_TAG_SIZE <= packetBuffer + numberOfBytes)
					{
						uint8_t messageTag[NET_MESSAGE_TAG_SIZE] = {0};
						ADVANCE_RECEIVE_BUFFER(&buffer, messageTag);
						
						if (messageTag[0] == 's' && messageTag[1] == 'n')
						{
							GameMessage message;
							message.type = QUIT_MESSAGE_TYPE;
							pushNetworkMessage(&gGameMessagesFromNet, message);
							
							needsToQuit = SDL_TRUE;
							
							break;
						}
						else if (messageTag[0] == 's' && messageTag[1] == 'r')
						{
							uint32_t packetNumber = 0;
							uint8_t slotID = 0;
							uint8_t characterLives = 0;
							
							if (buffer + sizeof(packetNumber) + sizeof(slotID) + sizeof(characterLives) <= packetBuffer + numberOfBytes)
							{
								ADVANCE_RECEIVE_BUFFER(&buffer, packetNumber);
								ADVANCE_RECEIVE_BUFFER(&buffer, slotID);
								ADVANCE_RECEIVE_BUFFER(&buffer, characterLives);
								
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
						else if (messageTag[0] == 'n' && messageTag[1] == 'w')
						{
							uint32_t packetNumber = 0;
							uint8_t numberOfWaitingPlayers = 0;
							
							if (buffer + sizeof(packetNumber) + sizeof(numberOfWaitingPlayers) <= packetBuffer + numberOfBytes)
							{
								ADVANCE_RECEIVE_BUFFER(&buffer, packetNumber);
								ADVANCE_RECEIVE_BUFFER(&buffer, numberOfWaitingPlayers);
								
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
						}
						else if (messageTag[0] == 'n' && messageTag[1] == 'n')
						{
							// net name
							uint32_t packetNumber = 0;
							uint8_t characterID = 0;
							
							if (buffer + sizeof(packetNumber) + sizeof(characterID) <= packetBuffer + numberOfBytes)
							{
								char *netName = calloc(MAX_USER_NAME_SIZE, 1);
								if (netName != NULL)
								{
									ADVANCE_RECEIVE_BUFFER(&buffer, packetNumber);
									ADVANCE_RECEIVE_BUFFER(&buffer, characterID);
									
									strncpy(netName, buffer, MAX_USER_NAME_SIZE - 1);
									buffer += MAX_USER_NAME_SIZE;
									
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
						}
						else if (messageTag[0] == 's' && messageTag[1] == 'g')
						{
							// start game
							uint32_t packetNumber = 0;
							ADVANCE_RECEIVE_BUFFER(&buffer, packetNumber);
							
							if (buffer + sizeof(packetNumber) <= packetBuffer + numberOfBytes)
							{
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
						}
						else if (messageTag[0] == 'g' && messageTag[1] == 's')
						{
							uint32_t packetNumber = 0;
							uint8_t gameStartNumber = 0;
							
							if (buffer + sizeof(packetNumber) + sizeof(gameStartNumber) <= packetBuffer + numberOfBytes)
							{
								ADVANCE_RECEIVE_BUFFER(&buffer, packetNumber);
								ADVANCE_RECEIVE_BUFFER(&buffer, gameStartNumber);
								
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
						}
						else if (messageTag[0] == 'm' && messageTag[1] == 'o')
						{
							uint32_t packetNumber = 0;
							uint8_t characterID = 0;
							uint8_t direction = 0;
							uint8_t pointing_direction = 0;
							uint32_t timestamp = 0;
							float x = 0.0f;
							float y = 0.0f;
							float z = 0.0f;
							
							if (buffer + sizeof(packetNumber) + sizeof(characterID) + sizeof(direction) + sizeof(pointing_direction) + sizeof(timestamp) + sizeof(x) + sizeof(y) + sizeof(z) <= packetBuffer + numberOfBytes)
							{
								ADVANCE_RECEIVE_BUFFER(&buffer, packetNumber);
								ADVANCE_RECEIVE_BUFFER(&buffer, characterID);
								ADVANCE_RECEIVE_BUFFER(&buffer, x);
								ADVANCE_RECEIVE_BUFFER(&buffer, y);
								ADVANCE_RECEIVE_BUFFER(&buffer, z);
								ADVANCE_RECEIVE_BUFFER(&buffer, direction);
								ADVANCE_RECEIVE_BUFFER(&buffer, pointing_direction);
								ADVANCE_RECEIVE_BUFFER(&buffer, timestamp);
								
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
						}
						else if (messageTag[0] == 'p' && messageTag[1] == 'k')
						{
							// character gets killed
							uint32_t packetNumber = 0;
							uint8_t characterID = 0;
							uint8_t characterLives = 0;
							
							if (buffer + sizeof(packetNumber) + sizeof(characterID) + sizeof(characterLives) <= packetBuffer + numberOfBytes)
							{
								ADVANCE_RECEIVE_BUFFER(&buffer, packetNumber);
								ADVANCE_RECEIVE_BUFFER(&buffer, characterID);
								ADVANCE_RECEIVE_BUFFER(&buffer, characterLives);
								
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
						}
						else if (messageTag[0] == 'c' && messageTag[1] == 'k')
						{
							// character's kills increase
							uint32_t packetNumber = 0;
							uint8_t characterID = 0;
							uint8_t characterKills = 0;
							
							if (buffer + sizeof(packetNumber) + sizeof(characterID) + sizeof(characterKills) <= packetBuffer + numberOfBytes)
							{
								ADVANCE_RECEIVE_BUFFER(&buffer, packetNumber);
								ADVANCE_RECEIVE_BUFFER(&buffer, characterID);
								ADVANCE_RECEIVE_BUFFER(&buffer, characterKills);
								
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
						}
						else if (messageTag[0] == 's' && messageTag[1] == 'w')
						{
							// shoot weapon
							uint32_t packetNumber = 0;
							uint8_t characterID = 0;
							float x = 0.0f;
							float y = 0.0f;
							int8_t pointing_direction = 0;
							
							if (buffer + sizeof(packetNumber) + sizeof(characterID) + sizeof(x) + sizeof(y) + sizeof(pointing_direction) <= packetBuffer + numberOfBytes)
							{
								ADVANCE_RECEIVE_BUFFER(&buffer, packetNumber);
								ADVANCE_RECEIVE_BUFFER(&buffer, characterID);
								ADVANCE_RECEIVE_BUFFER(&buffer, x);
								ADVANCE_RECEIVE_BUFFER(&buffer, y);
								ADVANCE_RECEIVE_BUFFER(&buffer, pointing_direction);
								
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
						}
						else if (messageTag[0] == 'c' && messageTag[1] == 't')
						{
							uint32_t packetNumber = 0;
							uint8_t characterID = 0;
							uint8_t tileIndex = 0;
							
							if (buffer + sizeof(packetNumber) + sizeof(characterID) + sizeof(tileIndex) <= packetBuffer + numberOfBytes)
							{
								ADVANCE_RECEIVE_BUFFER(&buffer, packetNumber);
								ADVANCE_RECEIVE_BUFFER(&buffer, characterID);
								ADVANCE_RECEIVE_BUFFER(&buffer, tileIndex);
								
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
						}
						else if (messageTag[0] == 't' && messageTag[1] == 'f')
						{
							uint32_t packetNumber = 0;
							uint8_t tileIndex = 0;
							int8_t dead = 0;
							
							if (buffer + sizeof(packetNumber) + sizeof(tileIndex) + sizeof(dead) <= packetBuffer + numberOfBytes)
							{
								ADVANCE_RECEIVE_BUFFER(&buffer, packetNumber);
								ADVANCE_RECEIVE_BUFFER(&buffer, tileIndex);
								ADVANCE_RECEIVE_BUFFER(&buffer, dead);
								
								if (packetNumber == triggerIncomingPacketNumber + 1 && tileIndex >= 0 && tileIndex < NUMBER_OF_TILES)
								{
									triggerIncomingPacketNumber++;
									
									GameMessage message;
									message.type = TILE_FALLING_DOWN_MESSAGE_TYPE;
									message.fallingTile.tileIndex = tileIndex;
									message.fallingTile.dead = (dead != 0);
									
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
						else if (messageTag[0] == 'r' && messageTag[1] == 't')
						{
							uint32_t packetNumber = 0;
							uint8_t tileIndex = 0;
							
							if (buffer + sizeof(packetNumber) + sizeof(tileIndex) <= packetBuffer + numberOfBytes)
							{
								ADVANCE_RECEIVE_BUFFER(&buffer, packetNumber);
								ADVANCE_RECEIVE_BUFFER(&buffer, tileIndex);
								
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
						}
						else if (messageTag[0] == 'n' && messageTag[1] == 'g')
						{
							// new game
							uint32_t packetNumber = 0;
							if (buffer + sizeof(packetNumber) <= packetBuffer + numberOfBytes)
							{
								ADVANCE_RECEIVE_BUFFER(&buffer, packetNumber);
								
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
						}
						else if (messageTag[0] == 'a' && messageTag[1] == 'k')
						{
							uint32_t packetNumber = 0;
							if (buffer + sizeof(packetNumber) <= packetBuffer + numberOfBytes)
							{
								ADVANCE_RECEIVE_BUFFER(&buffer, packetNumber);
								
								SDL_bool foundAck = SDL_FALSE;
								uint32_t maxPacketCount = receivedAckPacketCount < receivedAckPacketsCapacity ? receivedAckPacketCount : receivedAckPacketsCapacity;
								for (uint32_t packetIndex = 0; packetIndex < maxPacketCount; packetIndex++)
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
						}
						else if (messageTag[0] == 'p' && messageTag[1] == 'i')
						{
							// ping message
							uint32_t timestamp = 0;
							if (buffer + sizeof(timestamp) <= packetBuffer + numberOfBytes)
							{
								ADVANCE_RECEIVE_BUFFER(&buffer, timestamp);
								
								GameMessage pongMessage;
								pongMessage.type = PONG_MESSAGE_TYPE;
								pongMessage.pongTimestamp = timestamp;
								pushNetworkMessage(&gGameMessagesToNet, pongMessage);
							}
						}
						else if (messageTag[0] == 'p' && messageTag[1] == 'o')
						{
							// pong message
							uint32_t timestamp = 0;
							if (buffer + sizeof(timestamp) <= packetBuffer + numberOfBytes)
							{
								ADVANCE_RECEIVE_BUFFER(&buffer, timestamp);
								
								GameMessage message;
								message.type = PONG_MESSAGE_TYPE;
								message.pongTimestamp = timestamp;
								pushNetworkMessage(&gGameMessagesFromNet, message);
								
								lastPongReceivedTimestamp = SDL_GetTicks();
							}
						}
						else if (messageTag[0] == 'q' && messageTag[1] == 'u')
						{
							// quit
							GameMessage message;
							message.type = QUIT_MESSAGE_TYPE;
							pushNetworkMessage(&gGameMessagesFromNet, message);
							
							needsToQuit = SDL_TRUE;
							
							break;
						}
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

void initializeNetwork(void)
{
#ifdef WINDOWS
	WSADATA wsaData;
	
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
	{
		fprintf(stderr, "WSAStartup failed.\n");
    }
#endif
}

void deinitializeNetwork(void)
{
#ifdef WINDOWS
	WSACleanup();
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
static void cleanupStateFromNetwork(void)
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
	
	resetCharacterWins();
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
