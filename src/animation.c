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

#include "animation.h"
#include "collision.h"
#include "weapon.h"
#include "ai.h"
#include "network.h"
#include "audio.h"
#include "zgtime.h"
#include "globals.h"
#include "window.h"

#define TILE_FALLING_SPEED 25.4237f

// in seconds
#define STATS_WILL_APPEAR 4

#define CHARACTER_FALLING_SPEED 25.4237f

#define WEAPON_PROJECTILE_SPEED 30.0f

#define BEGIN_TILE_LAYER_ANIMATION 100
#define END_TILE_LAYER_ANIMATION 200

#define ANIMATION_TIME_ELAPSED_INTERVAL 0.0177 // in seconds

static int gTilesLayer[28];

typedef struct
{
	int colorIndex;
	int deathIndex;
	int animationTimer;
} TileLayerState;

static TileLayerState gTileLayerStates[2];

static float gSecondTimer =						0.0f;
static float gLastSecond =						0.0f;

static int gCurrentWinner =						0;
static int gStatsTimer =						0;

static double gTimeElapsedAccumulator = 0.0;

/* Functions */

static void colorTile(int tileIndex, int tileColoredCounter, Character *character, float currentTime);

static void crackTiles(float currentTime);

static void animateTilesAndPlayerRecovery(double timeDelta, ZGWindow *window, Character *player, float currentTime, GameState gameState);
static void moveWeapon(Weapon *weapon, double timeDelta);

static void firstTileLayerAnimation(ZGWindow *window, GameState gameState);
static void secondTileLayerAnimation(ZGWindow *window, GameState gameState);

static void loadFirstTileAnimationLayer(void);
static void loadSecondTileAnimationLayer(void);

static void collapseTiles(double timeDelta);
static void recoverDestroyedTiles(double timeDelta);

static void killCharacter(Input *characterInput, bool tutorial, double timeDelta);
static void recoverCharacter(Character *player);

static void sendPing(void);

static void clearPredictedColors(float currentTime);

void animate(ZGWindow *window, double timeDelta, GameState gameState)
{
	gSecondTimer += (float)timeDelta;
	
	bool tutorialState = (gameState == GAME_STATE_TUTORIAL);
	
	if (tutorialState && gTutorialCoverTimer > 0.0f)
	{
		gTutorialCoverTimer -= (float)timeDelta;
	}
	
	// Update gSecondTimer and change gLastSecond
	if ((int)gLastSecond != (int)gSecondTimer)
	{
		gLastSecond = gSecondTimer;
		
		if (tutorialState)
		{
			if (gTutorialStage == 0)
			{
				if (gSecondTimer >= 5.0f)
				{
					gTutorialStage = 1;
					gTutorialCoverTimer = 4.0f;
				}
			}
			else if ((gTutorialStage == 1 || gTutorialStage == 2) && gSecondTimer >= 6.0f)
			{
				float maxHumanDistance = 0.0f;
				if (gPinkBubbleGum.state == CHARACTER_HUMAN_STATE && maxHumanDistance < gPinkBubbleGum.distance)
				{
					maxHumanDistance = gPinkBubbleGum.distance;
				}
				
				if (gRedRover.state == CHARACTER_HUMAN_STATE && maxHumanDistance < gRedRover.distance)
				{
					maxHumanDistance = gRedRover.distance;
				}
				
				if (gGreenTree.state == CHARACTER_HUMAN_STATE && maxHumanDistance < gGreenTree.distance)
				{
					maxHumanDistance = gGreenTree.distance;
				}
				
				if (gBlueLightning.state == CHARACTER_HUMAN_STATE && maxHumanDistance < gBlueLightning.distance)
				{
					maxHumanDistance = gBlueLightning.distance;
				}
				
				float maxHumanNonstopDistance = 0.0f;
				if (gPinkBubbleGum.state == CHARACTER_HUMAN_STATE && maxHumanNonstopDistance < gPinkBubbleGum.nonstopDistance)
				{
					maxHumanNonstopDistance = gPinkBubbleGum.nonstopDistance;
				}
				
				if (gRedRover.state == CHARACTER_HUMAN_STATE && maxHumanNonstopDistance < gRedRover.nonstopDistance)
				{
					maxHumanNonstopDistance = gRedRover.nonstopDistance;
				}
				
				if (gGreenTree.state == CHARACTER_HUMAN_STATE && maxHumanNonstopDistance < gGreenTree.nonstopDistance)
				{
					maxHumanNonstopDistance = gGreenTree.nonstopDistance;
				}
				
				if (gBlueLightning.state == CHARACTER_HUMAN_STATE && maxHumanNonstopDistance < gBlueLightning.nonstopDistance)
				{
					maxHumanNonstopDistance = gBlueLightning.nonstopDistance;
				}
				
				if (maxHumanDistance >= 50.0f && maxHumanNonstopDistance >= 30.0f)
				{
					gTutorialStage = 3;
					// Allow firing now
					gGameHasStarted = true;
					gTutorialCoverTimer = 4.0f;
				}
				else if (gTutorialStage < 2 && maxHumanDistance >= 30.0f)
				{
					gTutorialStage = 2;
					gTutorialCoverTimer = 4.0f;
				}
			}
			else if (gTutorialStage == 3)
			{
				// AI can't fire in tutorial mode
				uint16_t numberOfFires = gPinkBubbleGum.numberOfFires + gRedRover.numberOfFires + gBlueLightning.numberOfFires + gGreenTree.numberOfFires;
				
				if (numberOfFires >= 2)
				{
					gTutorialStage = 4;
				}
			}
			else if (gTutorialStage == 4)
			{
				int totalLives = gPinkBubbleGum.lives + gRedRover.lives + gGreenTree.lives + gBlueLightning.lives;
				if (totalLives <= 2)
				{
					gTutorialStage = 5;
#if PLATFORM_IOS && !PLATFORM_TVOS
					gTutorialCoverTimer = 3.0f;
#endif
				}
			}
			else if (gTutorialStage == 5)
			{
				int totalLives = gPinkBubbleGum.lives + gRedRover.lives + gGreenTree.lives + gBlueLightning.lives;
				if (totalLives <= 1)
				{
					gTutorialStage = 6;
					gTutorialCoverTimer = 4.0f;
				}
			}
		}
		else
		{
			if (!gGameHasStarted && gPinkBubbleGum.netState != NETWORK_PENDING_STATE && gRedRover.netState != NETWORK_PENDING_STATE && gGreenTree.netState != NETWORK_PENDING_STATE && gBlueLightning.netState != NETWORK_PENDING_STATE)
			{
				if (gNetworkConnection && gNetworkConnection->type == NETWORK_SERVER_TYPE)
				{
					GameMessage message;
					message.type = GAME_START_NUMBER_UPDATE_MESSAGE_TYPE;
					message.gameStartNumber = gGameStartNumber - 1;
					sendToClients(0, &message);
				}
				
				if (!gNetworkConnection || gNetworkConnection->type == NETWORK_SERVER_TYPE)
				{
					gGameStartNumber--;
				}
			}
			
			if (gStatsTimer != 0)
			{
				gStatsTimer++;
				
				if (gStatsTimer >= STATS_WILL_APPEAR)
				{
					gGameWinner = gCurrentWinner;
				}
			}
		}
	}
	
	// Update time alive counters
	
	if (CHARACTER_IS_ALIVE(&gRedRover))
		gRedRover.time_alive += (float)timeDelta;
	
	if (CHARACTER_IS_ALIVE(&gGreenTree))
		gGreenTree.time_alive += (float)timeDelta;
	
	if (CHARACTER_IS_ALIVE(&gPinkBubbleGum))
		gPinkBubbleGum.time_alive += (float)timeDelta;
	
	if (CHARACTER_IS_ALIVE(&gBlueLightning))
		gBlueLightning.time_alive += (float)timeDelta;
	
	// Update inputs
	
	updateCharacterFromInput(&gRedRoverInput);
	updateCharacterFromInput(&gGreenTreeInput);
	updateCharacterFromInput(&gPinkBubbleGumInput);
	updateCharacterFromInput(&gBlueLightningInput);
	
	if (gameState != GAME_STATE_PAUSED)
	{
		updateCharacterFromAnyInput();
	}
	
	// Animate objects based on time delta
	updateAI(&gRedRover, gSecondTimer, !tutorialState, timeDelta);
	updateAI(&gGreenTree, gSecondTimer, !tutorialState, timeDelta);
	updateAI(&gPinkBubbleGum, gSecondTimer, !tutorialState, timeDelta);
	updateAI(&gBlueLightning, gSecondTimer, !tutorialState, timeDelta);
	
	// Weapons must have possibility of being updated before moving characters,
	// which could change their look direction
	fireCharacterWeapon(&gRedRover);
	fireCharacterWeapon(&gGreenTree);
	fireCharacterWeapon(&gPinkBubbleGum);
	fireCharacterWeapon(&gBlueLightning);
	
	moveCharacter(&gRedRover, timeDelta);
	moveCharacter(&gGreenTree, timeDelta);
	moveCharacter(&gPinkBubbleGum, timeDelta);
	moveCharacter(&gBlueLightning, timeDelta);
	
	moveWeapon(gRedRover.weap, timeDelta);
	moveWeapon(gGreenTree.weap, timeDelta);
	moveWeapon(gPinkBubbleGum.weap, timeDelta);
	moveWeapon(gBlueLightning.weap, timeDelta);
	
	killCharacter(&gRedRoverInput, tutorialState, timeDelta);
	killCharacter(&gGreenTreeInput, tutorialState, timeDelta);
	killCharacter(&gPinkBubbleGumInput, tutorialState, timeDelta);
	killCharacter(&gBlueLightningInput, tutorialState, timeDelta);
	
	clearPredictedColors(gSecondTimer);
	
	collapseTiles(timeDelta);
	
	animateTilesAndPlayerRecovery(timeDelta, window, &gRedRover, gSecondTimer, gameState);
	animateTilesAndPlayerRecovery(timeDelta, window, &gGreenTree, gSecondTimer, gameState);
	animateTilesAndPlayerRecovery(timeDelta, window, &gPinkBubbleGum, gSecondTimer, gameState);
	animateTilesAndPlayerRecovery(timeDelta, window, &gBlueLightning, gSecondTimer, gameState);
	crackTiles(gSecondTimer);
	
	recoverDestroyedTiles(timeDelta);
	
	sendPing();
	
	// Trigger events based on time elapsed
	gTimeElapsedAccumulator += timeDelta;
	while (gTimeElapsedAccumulator - ANIMATION_TIME_ELAPSED_INTERVAL >= 0.0)
	{
		firstTileLayerAnimation(window, gameState);
		secondTileLayerAnimation(window, gameState);
		
		recoverCharacter(&gRedRover);
		recoverCharacter(&gGreenTree);
		recoverCharacter(&gPinkBubbleGum);
		recoverCharacter(&gBlueLightning);
		
		gTimeElapsedAccumulator -= ANIMATION_TIME_ELAPSED_INTERVAL;
	}
}

static void sendPing(void)
{
	if (gNetworkConnection)
	{
		GameMessage message;
		message.type = PING_MESSAGE_TYPE;
		message.pingTimestamp = ZGGetTicks();
		
		if (gNetworkConnection->type == NETWORK_CLIENT_TYPE)
		{
			sendToServer(message);
		}
		else if (gNetworkConnection->type == NETWORK_SERVER_TYPE)
		{
			sendToClients(0, &message);
		}
	}
}

static void clearPredictedColors(float currentTime)
{
	if (gNetworkConnection != NULL && gNetworkConnection->type == NETWORK_CLIENT_TYPE)
	{
		for (int tileIndex = 0; tileIndex < NUMBER_OF_TILES; tileIndex++)
		{
			clearPredictedColorWithTime(tileIndex, currentTime);
		}
	}
}

static void crackTiles(float currentTime)
{
	for (int tileIndex = 0; tileIndex < NUMBER_OF_TILES; tileIndex++)
	{
		if (gTiles[tileIndex].crackedTime > 0.0f)
		{
			if (!gTiles[tileIndex].cracked && currentTime >= gTiles[tileIndex].crackedTime)
			{
				gTiles[tileIndex].cracked = true;
			}
			else if (gTiles[tileIndex].cracked && gTiles[tileIndex].state && currentTime >= gTiles[tileIndex].crackedTime + 2.5f)
			{
				gTiles[tileIndex].cracked = false;
				gTiles[tileIndex].crackedTime = 0.0f;
			}
		}
	}
}

/*
 * Change the color of the tile to the weap's color only if the color of the tile is at its default color and if that the tile's state exists
 */
static void colorTile(int tileIndex, int tileColoredCounter, Character *character, float currentTime)
{
	if (gTiles[tileIndex].state && gTiles[tileIndex].coloredID == NO_CHARACTER)
	{
		Weapon *weap = character->weap;
		
		gTiles[tileIndex].red = weap->red;
		gTiles[tileIndex].blue = weap->blue;
		gTiles[tileIndex].green = weap->green;
		
		gTiles[tileIndex].crackedTime = currentTime + 0.05f * (tileColoredCounter + 1);
		
		if (gNetworkConnection != NULL)
		{
			if (gNetworkConnection->type == NETWORK_SERVER_TYPE)
			{
				gTiles[tileIndex].coloredID = IDOfCharacter(character);
				gTiles[tileIndex].colorTime = currentTime;
				
				GameMessage message;
				message.type = COLOR_TILE_MESSAGE_TYPE;
				message.colorTile.characterID = IDOfCharacter(character);
				message.colorTile.tileIndex = tileIndex;
				
				sendToClients(0, &message);
			}
			else if (gNetworkConnection->type == NETWORK_CLIENT_TYPE)
			{
				gTiles[tileIndex].predictedColorID = IDOfCharacter(character);
				gTiles[tileIndex].predictedColorTime = currentTime;
			}
		}
		else
		{
			gTiles[tileIndex].coloredID = IDOfCharacter(character);
			gTiles[tileIndex].colorTime = currentTime;
		}
	}
}

static void moveWeapon(Weapon *weapon, double timeDelta)
{
	if (weapon->animationState)
	{
		if (weapon->direction == RIGHT)
		{
			weapon->x += WEAPON_PROJECTILE_SPEED * (float)timeDelta;
		}
		else if (weapon->direction == LEFT)
		{
			weapon->x -= WEAPON_PROJECTILE_SPEED * (float)timeDelta;
		}
		else if (weapon->direction == UP)
		{
			weapon->y += WEAPON_PROJECTILE_SPEED * (float)timeDelta;
		}
		else if (weapon->direction == DOWN)
		{
			weapon->y -= WEAPON_PROJECTILE_SPEED * (float)timeDelta;
		}
		
		weapon->z = INITIAL_WEAPON_Z + fabsf(cosf(weapon->timeFiring * 16.0f)) * 1.5f;
		weapon->timeFiring += (float)timeDelta;
	}
}

#define BEGIN_DESTROYING_TILES ((30 + 1) * ANIMATION_TIME_ELAPSED_INTERVAL)
#define RECOVERY_TIME_DELAY_DELTA (10 * ANIMATION_TIME_ELAPSED_INTERVAL)
#define CHARACTER_REGAIN_MOVEMENT (25 * ANIMATION_TIME_ELAPSED_INTERVAL)
#define END_CHARACTER_ANIMATION ((70 + 1) * ANIMATION_TIME_ELAPSED_INTERVAL)
#define NUM_ALPHA_FLASH_ITERATIONS 3
#define ALPHA_FLUCUATION 0.5f
static void animateTilesAndPlayerRecovery(double timeDelta, ZGWindow *window, Character *player, float currentTime, GameState gameState)
{
	if (player->weap->animationState)
	{
		if (player->animation_timer == 0.0 && ZGWindowHasFocus(window) && gAudioEffectsFlag && gameState != GAME_STATE_PAUSED)
		{
			playShootingSound(IDOfCharacter(player) - 1);
		}
		player->animation_timer += timeDelta;
		
		if ((gNetworkConnection == NULL && player->state == CHARACTER_HUMAN_STATE) || (gNetworkConnection != NULL && gNetworkConnection->character == player))
		{
			// Calculate player's alpha value based on current animation time
			
			// Calculate the size of each alpha chunk
			float alphaChunk = (((float)END_CHARACTER_ANIMATION - player->weap->compensation) / (float)NUM_ALPHA_FLASH_ITERATIONS);
			
			// Scale current time to alphaChunk
			float normalizedTime = fmodf((float)player->animation_timer, alphaChunk) / alphaChunk;
			
			// Calculate displacement using a triangle wave equation
			float displacement = 2.0f * ALPHA_FLUCUATION * fabsf(normalizedTime - floorf(normalizedTime + 0.5f));
			
			player->alpha = 1.0f - displacement;
		}
		
		/* First, color the tiles that are going to be destroyed */
		if (!player->coloredTiles)
		{
			// Get the location of where the player fired their weapon
			int tileLocation = getTileIndexLocation((int)player->weap->initialX, (int)player->weap->initialY);
			if (tileLocation >= 0 && tileLocation < NUMBER_OF_TILES)
			{
				player->player_loc = tileLocation;
				
				int currentTileIndex = tileLocation;
				
				if (player->weap->direction == RIGHT)
				{
					int tileColoredCounter = 0;
					while ((currentTileIndex = rightTileIndex(currentTileIndex)) != -1)
					{
						colorTile(currentTileIndex, tileColoredCounter, player, currentTime);
						tileColoredCounter++;
					}
				}
				else if (player->weap->direction == LEFT)
				{
					int tileColoredCounter = 0;
					while ((currentTileIndex = leftTileIndex(currentTileIndex)) != -1)
					{
						colorTile(currentTileIndex, tileColoredCounter, player, currentTime);
						tileColoredCounter++;
					}
				}
				else if (player->weap->direction == UP)
				{
					int tileColoredCounter = 0;
					while ((currentTileIndex = upTileIndex(currentTileIndex)) != -1)
					{
						colorTile(currentTileIndex, tileColoredCounter, player, currentTime);
						tileColoredCounter++;
					}
				}
				else if (player->weap->direction == DOWN)
				{
					int tileColoredCounter = 0;
					while ((currentTileIndex = downTileIndex(currentTileIndex)) != -1)
					{
						colorTile(currentTileIndex, tileColoredCounter, player, currentTime);
						tileColoredCounter++;
					}
				}
				
				player->coloredTiles = true;
			}
		}
		
		/* Then when animation_timer reaches BEGIN_DESTROYING_TILES, decrement the colored tile's z value by TILE_FALLING_SPEED, disable the tile's state, enable the tile's recovery_timer, and give the tile a proper recovery time delay */
		
		if (player->needTileLoc && player->animation_timer >= (BEGIN_DESTROYING_TILES - (double)player->weap->compensation) && player->player_loc != -1)
		{
			player->destroyedTileIndex = player->player_loc;
			player->needTileLoc = false;
			
			// no need to draw the weapon anymore
			player->weap->drawingState = false;
		}
		
		if (player->destroyedTileIndex != -1 && (!gNetworkConnection || gNetworkConnection->type == NETWORK_SERVER_TYPE))
		{
			if (player->weap->direction == RIGHT)
			{
				player->destroyedTileIndex = rightTileIndex(player->destroyedTileIndex);
			}
			else if (player->weap->direction == LEFT)
			{
				player->destroyedTileIndex = leftTileIndex(player->destroyedTileIndex);
			}
			else if (player->weap->direction == UP)
			{
				player->destroyedTileIndex = upTileIndex(player->destroyedTileIndex);
			}
			else if (player->weap->direction == DOWN)
			{
				player->destroyedTileIndex = downTileIndex(player->destroyedTileIndex);
			}
			
			if (player->destroyedTileIndex != -1 && gTiles[player->destroyedTileIndex].coloredID == IDOfCharacter(player))
			{
				gTiles[player->destroyedTileIndex].state = false;
				gTiles[player->destroyedTileIndex].z -= OBJECT_FALLING_STEP;
				
				if (ZGWindowHasFocus(window) && gAudioEffectsFlag && gameState != GAME_STATE_PAUSED)
				{
					playTileFallingSound();
				}
				
				GameMessage fallingMessage;
				fallingMessage.type = TILE_FALLING_DOWN_MESSAGE_TYPE;
				fallingMessage.fallingTile.tileIndex = player->destroyedTileIndex;
				fallingMessage.fallingTile.dead = false;
				sendToClients(0, &fallingMessage);
				
				/*
				 * Each destroyed_tile will get a different recovery_time delay.
				 * The first tile in the list gets a greater initial recovery_timer value than the second tile in the list, and so on.
				 */
				gTiles[player->destroyedTileIndex].recovery_timer = player->recovery_time_delay;
				player->recovery_time_delay -= RECOVERY_TIME_DELAY_DELTA;
			}
		}
		
		if (player->animation_timer >= (CHARACTER_REGAIN_MOVEMENT - player->weap->compensation) && CHARACTER_IS_ALIVE(player))
		{
			player->active = true;
		}
		
		// end the animation
		if (player->animation_timer >= (END_CHARACTER_ANIMATION - player->weap->compensation))
		{
			player->weap->animationState = false;
			player->weap->timeFiring = 0.0f;
			player->animation_timer = 0;
			
			player->alpha = 1.0f;
			player->needTileLoc = true;
			player->coloredTiles = false;
			player->destroyedTileIndex = -1;
			player->player_loc = -1;
			// A character can only destroy 7 tiles at once.
			player->recovery_time_delay = INITIAL_RECOVERY_TIME_DELAY;
		}
	}
}

/*
 * First layer of tiles to destroy (most outter one).
 * This animation is activated by setting gFirstLayerAnimationTimer = 1
 */
static void firstTileLayerAnimation(ZGWindow *window, GameState gameState)
{
	// Color the tiles dieing
	if (gTileLayerStates[0].colorIndex != -1 && gTileLayerStates[0].animationTimer > BEGIN_TILE_LAYER_ANIMATION)
	{
		if (gTiles[gTilesLayer[gTileLayerStates[0].colorIndex]].coloredID == NO_CHARACTER)
		{
			setDieingTileColor(gTilesLayer[gTileLayerStates[0].colorIndex]);
			
			if (ZGWindowHasFocus(window) && gAudioEffectsFlag && gameState != GAME_STATE_PAUSED)
			{
				playDieingStoneSound();
			}
		}
		else
		{
			gTiles[gTilesLayer[gTileLayerStates[0].colorIndex]].isDead = true;
		}
		
		gTileLayerStates[0].colorIndex++;
		
		if (gTileLayerStates[0].colorIndex == 28)
		{
			gTileLayerStates[0].colorIndex = -1;
		}
	}
	
	// Make the tiles fall down
	if (gTileLayerStates[0].deathIndex != -1 && gTileLayerStates[0].animationTimer > END_TILE_LAYER_ANIMATION)
	{
		if (!gTiles[gTilesLayer[gTileLayerStates[0].deathIndex]].isDead)
		{
			gTiles[gTilesLayer[gTileLayerStates[0].deathIndex]].z -= OBJECT_FALLING_STEP;
			gTiles[gTilesLayer[gTileLayerStates[0].deathIndex]].isDead = true;
			
			if (ZGWindowHasFocus(window) && gAudioEffectsFlag && gameState != GAME_STATE_PAUSED)
			{
				playTileFallingSound();
			}
			
			GameMessage fallingMessage;
			fallingMessage.type = TILE_FALLING_DOWN_MESSAGE_TYPE;
			fallingMessage.fallingTile.tileIndex = gTilesLayer[gTileLayerStates[0].deathIndex];
			fallingMessage.fallingTile.dead = true;
			sendToClients(0, &fallingMessage);
		}
		
		gTileLayerStates[0].deathIndex++;
		
		if (gTileLayerStates[0].deathIndex == 28)
		{
			gTileLayerStates[0].deathIndex = -1;
			loadSecondTileAnimationLayer();
			gTileLayerStates[0].animationTimer = -1;
		}
	}
	
	if (gTileLayerStates[0].animationTimer > 0)
		gTileLayerStates[0].animationTimer++;
}

/*
 * Second layer of tiles to destroy (second most outter one)
 * This animation is activated by setting gSecondLayerAnimationTimer = 1
 */
static void secondTileLayerAnimation(ZGWindow *window, GameState gameState)
{
	// Color the tiles dieing
	if (gTileLayerStates[1].colorIndex != -1 && gTileLayerStates[1].animationTimer > BEGIN_TILE_LAYER_ANIMATION)
	{
		if (gTiles[gTilesLayer[gTileLayerStates[1].colorIndex]].coloredID == NO_CHARACTER)
		{
			setDieingTileColor(gTilesLayer[gTileLayerStates[1].colorIndex]);
			
			if (ZGWindowHasFocus(window) && gAudioEffectsFlag && gameState != GAME_STATE_PAUSED)
			{
				playDieingStoneSound();
			}
		}
		else
		{
			gTiles[gTilesLayer[gTileLayerStates[1].colorIndex]].isDead = true;
		}
		
		gTileLayerStates[1].colorIndex++;
		
		if (gTileLayerStates[1].colorIndex == 20)
		{
			gTileLayerStates[1].colorIndex = -1;
		}
	}
	
	// Make the tiles fall down
	if (gTileLayerStates[1].deathIndex != -1 && gTileLayerStates[1].animationTimer > END_TILE_LAYER_ANIMATION)
	{
		if (!gTiles[gTilesLayer[gTileLayerStates[1].deathIndex]].isDead)
		{
			gTiles[gTilesLayer[gTileLayerStates[1].deathIndex]].z -= OBJECT_FALLING_STEP;
			gTiles[gTilesLayer[gTileLayerStates[1].deathIndex]].isDead = true;
			
			if (ZGWindowHasFocus(window) && gAudioEffectsFlag && gameState != GAME_STATE_PAUSED)
			{
				playTileFallingSound();
			}
			
			GameMessage fallingMessage;
			fallingMessage.type = TILE_FALLING_DOWN_MESSAGE_TYPE;
			fallingMessage.fallingTile.tileIndex = gTilesLayer[gTileLayerStates[1].deathIndex];
			fallingMessage.fallingTile.dead = true;
			sendToClients(0, &fallingMessage);
		}
		
		gTileLayerStates[1].deathIndex++;
		
		if (gTileLayerStates[1].deathIndex == 20)
		{
			gTileLayerStates[1].deathIndex = -1;
			gTileLayerStates[1].animationTimer = -1;
		}
	}
	
	if (gTileLayerStates[1].animationTimer > 0 && gTileLayerStates[0].animationTimer == -1)
		gTileLayerStates[1].animationTimer++;
}

static void collapseTiles(double timeDelta)
{
	int tileIndex;
	
	for (tileIndex = 0; tileIndex < NUMBER_OF_TILES; tileIndex++)
	{
		if (gTiles[tileIndex].z < TILE_ALIVE_Z && gTiles[tileIndex].z >= TILE_TERMINATING_Z)
		{
			gTiles[tileIndex].z -= TILE_FALLING_SPEED * (float)timeDelta;
		}
	}
}

void recoverDestroyedTile(int tileIndex)
{
	restoreDefaultTileColor(tileIndex);
	gTiles[tileIndex].coloredID = NO_CHARACTER;
	gTiles[tileIndex].colorTime = 0;
	gTiles[tileIndex].cracked = false;
	gTiles[tileIndex].crackedTime = 0.0f;
	gTiles[tileIndex].z = TILE_ALIVE_Z;
	gTiles[tileIndex].state = true;
	gTiles[tileIndex].recovery_timer = 0.0;
}

/* To activate a recovery of a tile, set its recover_timer to a value greater than 0 */
#define TILE_SPAWN_TIME ((200 + 1) * ANIMATION_TIME_ELAPSED_INTERVAL)
static void recoverDestroyedTiles(double timeDelta)
{
	if (gNetworkConnection && gNetworkConnection->type == NETWORK_CLIENT_TYPE)
	{
		return;
	}
	
	for (int tileIndex = 0; tileIndex < NUMBER_OF_TILES; tileIndex++)
	{
		if (gTiles[tileIndex].recovery_timer > 0.0)
		{
			gTiles[tileIndex].recovery_timer += timeDelta;
		}
		
		// it's time to recover!
		if (gTiles[tileIndex].recovery_timer >= TILE_SPAWN_TIME && !gTiles[tileIndex].isDead)
		{
			recoverDestroyedTile(tileIndex);
			
			GameMessage recoverMessage;
			recoverMessage.type = RECOVER_TILE_MESSAGE_TYPE;
			recoverMessage.recoverTile.tileIndex = tileIndex;
			sendToClients(0, &recoverMessage);
		}
	}
}

void decideWhetherToMakeAPlayerAWinner(Character *player)
{
	/*
	 * Every time a player loses all of their lives, a layer of tiles become into stone and fall down.
	 * There has to be at least two players alive, meaning that we'll only ever need to cut down two layers of tiles since there can only be four players max.
	 */
	
	if (player->lives == 0)
	{
		int numPlayersAlive = 0;
		Character *winnerCharacter = NULL;
		
		if (gRedRover.lives > 0)
		{
			numPlayersAlive++;
			winnerCharacter = &gRedRover;
		}
		
		if (gGreenTree.lives > 0)
		{
			numPlayersAlive++;
			winnerCharacter = &gGreenTree;
		}
		
		if (gBlueLightning.lives > 0)
		{
			numPlayersAlive++;
			winnerCharacter = &gBlueLightning;
		}
		
		if (gPinkBubbleGum.lives > 0)
		{
			numPlayersAlive++;
			winnerCharacter = &gPinkBubbleGum;
		}
		
		if (numPlayersAlive >= 2)
		{
			if (gTileLayerStates[0].animationTimer == 0)
			{
				gTileLayerStates[0].animationTimer = 1;
			}
			else if (gTileLayerStates[1].animationTimer == 0)
			{
				gTileLayerStates[1].animationTimer = 1;
			}
		}
		else if (numPlayersAlive == 1)
		{
			// check if the winner is really about to die, in that case, we'll need to see who is the highest up to determine the winner
			int winnerTileIndex = getTileIndexLocation((int)winnerCharacter->x, (int)winnerCharacter->y);
			bool winnerIsReadyForDeath = (winnerTileIndex >= 0 && winnerTileIndex < NUMBER_OF_TILES && gTiles[winnerTileIndex].z < TILE_ALIVE_Z && winnerCharacter->z > CHARACTER_TERMINATING_Z && winnerCharacter->lives == 1);
			if (winnerIsReadyForDeath)
			{
				if (winnerCharacter != &gRedRover && gRedRover.z > winnerCharacter->z)
				{
					winnerCharacter = &gRedRover;
				}
				else if (winnerCharacter != &gGreenTree && gGreenTree.z > winnerCharacter->z)
				{
					winnerCharacter = &gGreenTree;
				}
				else if (winnerCharacter != &gBlueLightning && gBlueLightning.z > winnerCharacter->z)
				{
					winnerCharacter = &gBlueLightning;
				}
				else if (winnerCharacter != &gPinkBubbleGum && gPinkBubbleGum.z > winnerCharacter->z)
				{
					winnerCharacter = &gPinkBubbleGum;
				}
			}
			// Make sure a winner doesn't exist yet
			if (gGameWinner == NO_CHARACTER && IDOfCharacter(winnerCharacter) != NO_CHARACTER)
			{
				gCurrentWinner = IDOfCharacter(winnerCharacter);
				winnerCharacter->wins++;
				gStatsTimer++;
			}
		}
	}
}

void prepareCharactersDeath(Character *player)
{	
	player->active = false;
}

/*
 * The characterInput is passed because we have to turn off the character's inputs
 */
static void killCharacter(Input *characterInput, bool tutorial, double timeDelta)
{
	Character *player = characterFromInput(characterInput);
	
	int location = getTileIndexLocation((int)player->x, (int)player->y);
	
	// Make the character fall down if the tile is falling down
	if (location >= 0 && location < NUMBER_OF_TILES && gTiles[location].z < TILE_ALIVE_Z && CHARACTER_IS_ALIVE(player) && (!gNetworkConnection || gNetworkConnection->type == NETWORK_SERVER_TYPE))
	{
		prepareCharactersDeath(player);
		
		player->time_alive = 0.0f;
		
		player->lives--;
		player->active = false;
		
		if (gNetworkConnection && gNetworkConnection->type == NETWORK_SERVER_TYPE)
		{
			GameMessage message;
			message.type = CHARACTER_DIED_UPDATE_MESSAGE_TYPE;
			message.diedUpdate.characterID = IDOfCharacter(player);
			message.diedUpdate.characterLives = player->lives;
			
			sendToClients(0, &message);
		}
		
		if (!tutorial)
		{
			decideWhetherToMakeAPlayerAWinner(player);
		}
		
		// who killed me?
		Character *killer = NULL;
		if (gTiles[location].coloredID == RED_ROVER)
		{
			killer = &gRedRover;
		}
		else if (gTiles[location].coloredID == PINK_BUBBLE_GUM)
		{
			killer = &gPinkBubbleGum;
		}
		else if (gTiles[location].coloredID == BLUE_LIGHTNING)
		{
			killer = &gBlueLightning;
		}
		else if (gTiles[location].coloredID == GREEN_TREE)
		{
			killer = &gGreenTree;
		}
		
		if (killer)
		{
			(killer->kills)++;
			
			if (gNetworkConnection && gNetworkConnection->type == NETWORK_SERVER_TYPE)
			{
				GameMessage message;
				message.type = CHARACTER_KILLED_UPDATE_MESSAGE_TYPE;
				message.killedUpdate.characterID = IDOfCharacter(killer);
				message.killedUpdate.kills = killer->kills;
				
				sendToClients(0, &message);
			}
		}
		
		player->z -= OBJECT_FALLING_STEP;
	}
	
	if (!CHARACTER_IS_ALIVE(player) && player->z > CHARACTER_TERMINATING_Z)
	{
		player->z -= CHARACTER_FALLING_SPEED * (float)timeDelta;
		player->recovery_timer = 1;
	}
}

static void recoverCharacter(Character *player)
{
	if (!player->lives || (gNetworkConnection && gNetworkConnection->type == NETWORK_CLIENT_TYPE))
	{
		return;
	}
	
	if (player->recovery_timer > 0)
	{
		player->recovery_timer++;
	}
	
	static const int CHARACTER_SPAWN_TIME = 2;
	
	if (player->recovery_timer > CHARACTER_SPAWN_TIME)
	{
		if (player->lives)
		{
			spawnCharacter(player);
			player->active = true;
		}
		
		player->recovery_timer = 0;
	}
}

/* Most outter tile layer */
static void loadFirstTileAnimationLayer(void)
{
	gTilesLayer[0] = 56;
	gTilesLayer[1] = 57;
	gTilesLayer[2] = 58;
	gTilesLayer[3] = 59;
	gTilesLayer[4] = 60;
	gTilesLayer[5] = 61;
	gTilesLayer[6] = 62;
	gTilesLayer[7] = 63;
	
	gTilesLayer[8] = 55;
	gTilesLayer[9] = 47;
	gTilesLayer[10] = 39;
	gTilesLayer[11] = 31;
	gTilesLayer[12] = 23;
	gTilesLayer[13] = 15;
	gTilesLayer[14] = 7;
	
	gTilesLayer[15] = 6;
	gTilesLayer[16] = 5;
	gTilesLayer[17] = 4;
	gTilesLayer[18] = 3;
	gTilesLayer[19] = 2;
	gTilesLayer[20] = 1;
	gTilesLayer[21] = 0;
	
	gTilesLayer[22] = 8;
	gTilesLayer[23] = 16;
	gTilesLayer[24] = 24;
	gTilesLayer[25] = 32;
	gTilesLayer[26] = 40;
	gTilesLayer[27] = 48;
}

/* Second most outter tile layer */
static void loadSecondTileAnimationLayer(void)
{
	gTilesLayer[0] = 49;
	gTilesLayer[1] = 50;
	gTilesLayer[2] = 51;
	gTilesLayer[3] = 52;
	gTilesLayer[4] = 53;
	gTilesLayer[5] = 54;
	
	gTilesLayer[6] = 46;
	gTilesLayer[7] = 38;
	gTilesLayer[8] = 30;
	gTilesLayer[9] = 22;
	gTilesLayer[10] = 14;
	
	gTilesLayer[11] = 13;
	gTilesLayer[12] = 12;
	gTilesLayer[13] = 11;
	gTilesLayer[14] = 10;
	gTilesLayer[15] = 9;
	
	gTilesLayer[16] = 17;
	gTilesLayer[17] = 25;
	gTilesLayer[18] = 33;
	gTilesLayer[19] = 41;
}

void startAnimation(void)
{
	loadFirstTileAnimationLayer();
}

void endAnimation(void)
{
	gSecondTimer = 0.0f;
	gLastSecond = 0.0f;
	gTimeElapsedAccumulator = 0.0;
	
	gTileLayerStates[0].colorIndex = 0;
	gTileLayerStates[0].deathIndex = 0;
	gTileLayerStates[0].animationTimer = 0;
	
	gTileLayerStates[1].colorIndex = 0;
	gTileLayerStates[1].deathIndex = 0;
	gTileLayerStates[1].animationTimer = 0;
	
	// stop weapon animation and drawing
	gRedRover.weap->drawingState = false;
	gRedRover.weap->animationState = false;
	gRedRover.weap->timeFiring = 0.0f;
	
	gGreenTree.weap->drawingState = false;
	gGreenTree.weap->animationState = false;
	gGreenTree.weap->timeFiring = 0.0f;
	
	gPinkBubbleGum.weap->drawingState = false;
	gPinkBubbleGum.weap->animationState = false;
	gPinkBubbleGum.weap->timeFiring = 0.0f;
	
	gBlueLightning.weap->drawingState = false;
	gBlueLightning.weap->animationState = false;
	gBlueLightning.weap->timeFiring = 0.0f;
	
	gStatsTimer = 0;
	gCurrentWinner = 0;
}
