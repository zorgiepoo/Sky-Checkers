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
#include "utilities.h"
#include "weapon.h"
#include "ai.h"
#include "network.h"
#include "audio.h"

#define OBJECT_FALLING_STEP 0.2f

#define TILE_FALLING_SPEED 25.4237f

// in seconds
#define STATS_WILL_APPEAR 4

#define CHARACTER_FALLING_SPEED 25.4237f

#define WEAPON_PROJECTILE_SPEED 45.1977f

#define BEGIN_TILE_LAYER_ANIMATION 100
#define END_TILE_LAYER_ANIMATION 200

#define ANIMATION_TIME_ELAPSED_INTERVAL 0.0177 // in seconds

static int gTilesLayer[28];

static int gLayerColorIndex =					0;
static int gLayerDeathIndex =					0;
static int gLayerTwoColorIndex =				0;
static int gLayerTwoDeathIndex =				0;

static float gSecondTimer =						0.0f;
static float gLastSecond =						0.0f;

static int gFirstLayerAnimationTimer =			0;
static int gSecondLayerAnimationTimer =			0;

static int gCurrentWinner =						0;
static int gStatsTimer =						0;

static double gTimeElapsedAccumulator = 0.0;

/* Functions */

static void colorTile(Tile *tile, Weapon *weap);

static void animateTilesAndPlayerRecovery(SDL_Window *window, Character *player);
static void moveWeapon(Weapon *weapon, double timeDelta);

static void firstTileLayerAnimation(SDL_Window *window);
static void secondTileLayerAnimation(SDL_Window *window);

static void loadFirstTileAnimationLayer(void);
static void loadSecondTileAnimationLayer(void);

static void collapseTiles(double timeDelta);
static void recoverDestroyedTiles(void);

static void killCharacter(Input *characterInput, double timeDelta);
static void recoverCharacter(Character *player);

void animate(SDL_Window *window, double timeDelta)
{
	gSecondTimer += timeDelta;
	
	// Update gSecondTimer and change gLastSecond
	if ((int)gLastSecond != (int)gSecondTimer)
	{
		gLastSecond = gSecondTimer;
		
		if (!gGameHasStarted && gPinkBubbleGum.netState != NETWORK_PENDING_STATE && gRedRover.netState != NETWORK_PENDING_STATE && gGreenTree.netState != NETWORK_PENDING_STATE && gBlueLightning.netState != NETWORK_PENDING_STATE)
		{
			if (gNetworkConnection && gNetworkConnection->type == NETWORK_SERVER_TYPE)
			{
				sendToClients(0, "gs%i", gGameStartNumber - 1);
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
		
		// Make sure character is on checkerboard
		
		if (gRedRover.z == 2.0)
			gRedRover.time_alive++;
		
		if (gGreenTree.z == 2.0)
			gGreenTree.time_alive++;
		
		if (gPinkBubbleGum.z == 2.0)
			gPinkBubbleGum.time_alive++;
		
		if (gBlueLightning.z == 2.0)
			gBlueLightning.time_alive++;
	}
	
	// Animate objects based on time delta
	
	moveCharacterFromInput(&gRedRoverInput, timeDelta);
	moveCharacterFromInput(&gGreenTreeInput, timeDelta);
	moveCharacterFromInput(&gPinkBubbleGumInput, timeDelta);
	moveCharacterFromInput(&gBlueLightningInput, timeDelta);
	
	moveAI(&gRedRover, (int)gSecondTimer, timeDelta);
	moveAI(&gGreenTree, (int)gSecondTimer, timeDelta);
	moveAI(&gPinkBubbleGum, (int)gSecondTimer, timeDelta);
	moveAI(&gBlueLightning, (int)gSecondTimer, timeDelta);
	
	moveWeapon(gRedRover.weap, timeDelta);
	moveWeapon(gGreenTree.weap, timeDelta);
	moveWeapon(gPinkBubbleGum.weap, timeDelta);
	moveWeapon(gBlueLightning.weap, timeDelta);
	
	killCharacter(&gRedRoverInput, timeDelta);
	killCharacter(&gGreenTreeInput, timeDelta);
	killCharacter(&gPinkBubbleGumInput, timeDelta);
	killCharacter(&gBlueLightningInput, timeDelta);
	
	collapseTiles(timeDelta);
	
	// Trigger events based on time elapsed
	gTimeElapsedAccumulator += timeDelta;
	while (gTimeElapsedAccumulator - ANIMATION_TIME_ELAPSED_INTERVAL >= 0.0)
	{
		firstTileLayerAnimation(window);
		secondTileLayerAnimation(window);
		
		animateTilesAndPlayerRecovery(window, &gRedRover);
		animateTilesAndPlayerRecovery(window, &gGreenTree);
		animateTilesAndPlayerRecovery(window, &gPinkBubbleGum);
		animateTilesAndPlayerRecovery(window, &gBlueLightning);
		
		recoverDestroyedTiles();
		
		recoverCharacter(&gRedRover);
		recoverCharacter(&gGreenTree);
		recoverCharacter(&gPinkBubbleGum);
		recoverCharacter(&gBlueLightning);
		
		gTimeElapsedAccumulator -= ANIMATION_TIME_ELAPSED_INTERVAL;
	}
}

/*
 * Change the color of the tile to the weap's color only if the color of the tile is at its default color and if that the tile's state exists
 */
static void colorTile(Tile *tile, Weapon *weap)
{
	if (tile->state					&&
		tile->red == tile->d_red	&&
		tile->blue == tile->d_blue	&&
		tile->green == tile->d_green)
	{
		tile->red = weap->red;
		tile->blue = weap->blue;
		tile->green = weap->green;
	}
}

static void moveWeapon(Weapon *weapon, double timeDelta)
{
	if (weapon->animationState)
	{
		if (weapon->direction == RIGHT)
		{
			weapon->x += WEAPON_PROJECTILE_SPEED * timeDelta;
		}
		else if (weapon->direction == LEFT)
		{
			weapon->x -= WEAPON_PROJECTILE_SPEED * timeDelta;
		}
		else if (weapon->direction == UP)
		{
			weapon->y += WEAPON_PROJECTILE_SPEED * timeDelta;
		}
		else if (weapon->direction == DOWN)
		{
			weapon->y -= WEAPON_PROJECTILE_SPEED * timeDelta;
		}
	}
}

static void animateTilesAndPlayerRecovery(SDL_Window *window, Character *player)
{
	if (player->weap->animationState)
	{
		if (player->animation_timer == 0 && ((SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0) && gAudioEffectsFlag)
		{
			playShootingSound();
		}
		player->animation_timer++;
		
		/* First, color the tiles that are going to be destroyed */
		if (!player->coloredTiles)
		{
			Tile *currentTile;
			
			// Get the location of where the player is
			player->player_loc = getTileIndexLocation((int)player->x, (int)player->y);
			
			currentTile = &gTiles[player->player_loc];
			
			if (player->weap->direction == RIGHT)
			{
				while ((currentTile = currentTile->right) != NULL)
				{
					colorTile(currentTile, player->weap);
				}
			}
			else if (player->weap->direction == LEFT)
			{
				while ((currentTile = currentTile->left) != NULL)
				{
					colorTile(currentTile, player->weap);
				}
			}
			else if (player->weap->direction == UP)
			{
				while ((currentTile = currentTile->up) != NULL)
				{
					colorTile(currentTile, player->weap);
				}
			}
			else if (player->weap->direction == DOWN)
			{
				while ((currentTile = currentTile->down) != NULL)
				{
					colorTile(currentTile, player->weap);
				}
			}
			
			player->coloredTiles = SDL_TRUE;
		}
		
		/* Then when animation_timer reaches BEGIN_DESTROYING_TILES, decrement the colored tile's z value by TILE_FALLING_SPEED, disable the tile's state, enable the tile's recovery_timer, and give the tile a proper recovery time delay */
		static const int BEGIN_DESTROYING_TILES =		30;
		
		if (player->needTileLoc && player->animation_timer > BEGIN_DESTROYING_TILES)
		{
			player->destroyed_tile = &gTiles[player->player_loc];
			player->needTileLoc = SDL_FALSE;
			
			// no need to draw the weapon anymore
			player->weap->drawingState = SDL_FALSE;
		}
		
		static const int RECOVERY_TIME_DELAY_DELTA =	10;
		
		if (player->destroyed_tile != NULL)
		{
			if (player->weap->direction == RIGHT)
			{
				player->destroyed_tile = player->destroyed_tile->right;
			}
			else if (player->weap->direction == LEFT)
			{
				player->destroyed_tile = player->destroyed_tile->left;
			}
			else if (player->weap->direction == UP)
			{
				player->destroyed_tile = player->destroyed_tile->up;
			}
			else if (player->weap->direction == DOWN)
			{
				player->destroyed_tile = player->destroyed_tile->down;
			}
			
			if (player->destroyed_tile != NULL						&&
				player->destroyed_tile->red == player->weap->red	&&
				player->destroyed_tile->blue == player->weap->blue	&&
				player->destroyed_tile->green == player->weap->green)
			{
				player->destroyed_tile->state = SDL_FALSE;
				player->destroyed_tile->z -= OBJECT_FALLING_STEP;
				
				if (((SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0) && gAudioEffectsFlag)
				{
					playTileFallingSound();
				}
				
				/*
				 * Each destroyed_tile will get a different recovery_time delay.
				 * The first tile in the list gets a greater initial recovery_timer value than the second tile in the list, and so on.
				 */
				player->destroyed_tile->recovery_timer = player->recovery_time_delay;
				player->recovery_time_delay -= RECOVERY_TIME_DELAY_DELTA;
			}
		}
		
		static const int CHARACTER_REGAIN_MOVEMENT = 25;
		
		if (player->animation_timer == CHARACTER_REGAIN_MOVEMENT)
		{
			player->direction = player->backup_direction;
		}
		
		static const int END_CHARACTER_ANIMATION = 70;
			
		// end the animation
		if (player->animation_timer > END_CHARACTER_ANIMATION)
		{
			player->weap->animationState = SDL_FALSE;
			player->animation_timer = 0;
			
			player->needTileLoc = SDL_TRUE;
			player->coloredTiles = SDL_FALSE;
			player->destroyed_tile = NULL;
			player->player_loc = 0;
			/*
			 * A character can only destroy 7 tiles at once.
			 * Setting the recovery_time_delay to 6 * RECOVERY_TIME_DELAY_DELTA + 1 gaurantees that each tile in the list will be activated -- that is, have a recovery_timer value greater than zero.
			 */
			player->recovery_time_delay = (6 * RECOVERY_TIME_DELAY_DELTA) + 1;
		}
	}
}

/*
 * First layer of tiles to destroy (most outter one).
 * This animation is activated by setting gFirstLayerAnimationTimer = 1
 */
static void firstTileLayerAnimation(SDL_Window *window)
{
	// Color the tiles gray
	if (gLayerColorIndex != -1 && gFirstLayerAnimationTimer > BEGIN_TILE_LAYER_ANIMATION)
	{
		if (gTiles[gTilesLayer[gLayerColorIndex]].red == gTiles[gTilesLayer[gLayerColorIndex]].d_red		&&
			gTiles[gTilesLayer[gLayerColorIndex]].green == gTiles[gTilesLayer[gLayerColorIndex]].d_green	&&
			gTiles[gTilesLayer[gLayerColorIndex]].blue == gTiles[gTilesLayer[gLayerColorIndex]].d_blue)
		{
			// RGB { 0.31, 0.33, 0.36 } == Grayish color
			gTiles[gTilesLayer[gLayerColorIndex]].red = 0.31f;
			gTiles[gTilesLayer[gLayerColorIndex]].green = 0.33f;
			gTiles[gTilesLayer[gLayerColorIndex]].blue = 0.36f;
			
			if (((SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0) && gAudioEffectsFlag)
			{
				playGrayStoneColorSound();
			}
		}
		else
		{
			gTiles[gTilesLayer[gLayerColorIndex]].isDead = SDL_TRUE;
		}
		
		gLayerColorIndex++;
		
		if (gLayerColorIndex == 28)
		{
			gLayerColorIndex = -1;
		}
	}
	
	// Make the tiles fall down
	if (gLayerDeathIndex != -1 && gFirstLayerAnimationTimer > END_TILE_LAYER_ANIMATION)
	{
		if (!gTiles[gTilesLayer[gLayerDeathIndex]].isDead)
		{
			gTiles[gTilesLayer[gLayerDeathIndex]].z -= OBJECT_FALLING_STEP;
			gTiles[gTilesLayer[gLayerDeathIndex]].isDead = SDL_TRUE;
			
			if (((SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0) && gAudioEffectsFlag)
			{
				playTileFallingSound();
			}
		}
		
		gLayerDeathIndex++;
		
		if (gLayerDeathIndex == 28)
		{
			gLayerDeathIndex = -1;
			loadSecondTileAnimationLayer();
			gFirstLayerAnimationTimer = -1;
		}
	}
	
	if (gFirstLayerAnimationTimer > 0)
		gFirstLayerAnimationTimer++;
}

/*
 * Second layer of tiles to destroy (second most outter one)
 * This animation is activated by setting gSecondLayerAnimationTimer = 1
 */
static void secondTileLayerAnimation(SDL_Window *window)
{
	// Color the tiles gray
	if (gLayerTwoColorIndex != -1 && gSecondLayerAnimationTimer > BEGIN_TILE_LAYER_ANIMATION)
	{
		if (gTiles[gTilesLayer[gLayerTwoColorIndex]].red == gTiles[gTilesLayer[gLayerTwoColorIndex]].d_red		&&
			gTiles[gTilesLayer[gLayerTwoColorIndex]].green == gTiles[gTilesLayer[gLayerTwoColorIndex]].d_green	&&
			gTiles[gTilesLayer[gLayerTwoColorIndex]].blue == gTiles[gTilesLayer[gLayerTwoColorIndex]].d_blue)
		{
			// RGB { 0.31, 0.33, 0.36 } == Grayish color
			gTiles[gTilesLayer[gLayerTwoColorIndex]].red = 0.31f;
			gTiles[gTilesLayer[gLayerTwoColorIndex]].green = 0.33f;
			gTiles[gTilesLayer[gLayerTwoColorIndex]].blue = 0.36f;
			
			if (((SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0) && gAudioEffectsFlag)
			{
				playGrayStoneColorSound();
			}
		}
		else
		{
			gTiles[gTilesLayer[gLayerTwoColorIndex]].isDead = SDL_TRUE;
		}
		
		gLayerTwoColorIndex++;
		
		if (gLayerTwoColorIndex == 20)
		{
			gLayerTwoColorIndex = -1;
		}
	}
	
	// Make the tiles fall down
	if (gLayerTwoDeathIndex != -1 && gSecondLayerAnimationTimer > END_TILE_LAYER_ANIMATION)
	{
		if (!gTiles[gTilesLayer[gLayerTwoDeathIndex]].isDead)
		{
			gTiles[gTilesLayer[gLayerTwoDeathIndex]].z -= OBJECT_FALLING_STEP;
			gTiles[gTilesLayer[gLayerTwoDeathIndex]].isDead = SDL_TRUE;
			
			if (((SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0) && gAudioEffectsFlag)
			{
				playTileFallingSound();
			}
		}
		
		gLayerTwoDeathIndex++;
		
		if (gLayerTwoDeathIndex == 20)
		{
			gLayerTwoDeathIndex = -1;
			gSecondLayerAnimationTimer = -1;
		}
	}
	
	if (gSecondLayerAnimationTimer > 0 && gFirstLayerAnimationTimer == -1)
		gSecondLayerAnimationTimer++;
}

static void collapseTiles(double timeDelta)
{
	int tileIndex;
	
	for (tileIndex = 0; tileIndex < NUMBER_OF_TILES; tileIndex++)
	{
		if (gTiles[tileIndex].z < TILE_ALIVE_Z)
		{
			gTiles[tileIndex].z -= TILE_FALLING_SPEED * timeDelta;
		}
	}
}

/* To activate a recovery of a tile, set its recover_timer to a value greater than 0 */
static void recoverDestroyedTiles(void)
{
	int tileIndex;
	
	for (tileIndex = 0; tileIndex < NUMBER_OF_TILES; tileIndex++)
	{
		if (gTiles[tileIndex].recovery_timer > 0)
		{
			gTiles[tileIndex].recovery_timer++;
		}
		
		static const int TILE_SPAWN_TIME = 200;
		
		// it's time to recover!
		if (gTiles[tileIndex].recovery_timer > TILE_SPAWN_TIME && !gTiles[tileIndex].isDead)
		{
			gTiles[tileIndex].red = gTiles[tileIndex].d_red;
			gTiles[tileIndex].green = gTiles[tileIndex].d_green;
			gTiles[tileIndex].blue = gTiles[tileIndex].d_blue;
			gTiles[tileIndex].z = TILE_ALIVE_Z;
			gTiles[tileIndex].state = SDL_TRUE;
			gTiles[tileIndex].recovery_timer = 0;
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
			if (gFirstLayerAnimationTimer == 0)
			{
				gFirstLayerAnimationTimer = 1;
			}
			else if (gSecondLayerAnimationTimer == 0)
			{
				gSecondLayerAnimationTimer = 1;
			}
		}
		else if (numPlayersAlive == 1)
		{
			// check if the winner is really about to die, in that case, we'll need to see who is the highest up to determine the winner
			SDL_bool winnerIsReadyForDeath = (gTiles[getTileIndexLocation((int)winnerCharacter->x, (int)winnerCharacter->y)].z < TILE_ALIVE_Z && winnerCharacter->z >= -70.0 && winnerCharacter->lives == 1);
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
	if (gNetworkConnection && gNetworkConnection->type == NETWORK_CLIENT_TYPE)
	{
		/*
		 * Make sure to turn everyone's inputs off in a networked game for clients.
		 * This can lead to a potentional bug if we don't in that when the character is recovered, an input may be automatically invoked, especially when dealing with joysticks
		 */
		
		turnInputOff(&gRedRoverInput);
		turnInputOff(&gBlueLightningInput);
		turnInputOff(&gGreenTreeInput);
		turnInputOff(&gPinkBubbleGumInput);
	}
	
	player->backup_direction = player->direction;
	player->direction = NO_DIRECTION;
}

/*
 * The characterInput is passed because we have to turn off the character's inputs
 */
static void killCharacter(Input *characterInput, double timeDelta)
{
	// client's don't kill characters...
	// server tells them when to kill!
	if (gNetworkConnection && gNetworkConnection->type == NETWORK_CLIENT_TYPE)
	{
		return;
	}
	
	Character *player = characterFromInput(characterInput);
	
	int location = getTileIndexLocation((int)player->x, (int)player->y);
	
	// Make the character fall down if the tile is falling down
	if (gTiles[location].z < TILE_ALIVE_Z && player->z >= -70.0)
	{
		if (player->direction && !player->weap->animationState)
		{
			prepareCharactersDeath(player);
			
			/*
			 * Make sure to turn the character's inputs off.
			 * This can lead to a potentional bug if we don't in that when the character is recovered, an input may be automatically invoked, especially when dealing with joysticks
			 */
			
			turnInputOff(characterInput);
			
			player->time_alive = 0;
			
			player->lives--;
			
			if (gNetworkConnection && gNetworkConnection->type == NETWORK_SERVER_TYPE)
			{
				sendToClients(0, "pk%i %i", IDOfCharacter(player), player->lives);
			}
			
			decideWhetherToMakeAPlayerAWinner(player);
			
			int location = getTileIndexLocation((int)player->x, (int)player->y);
			
			// who killed me?
			Character *killer = NULL;
			if (fabs(gTiles[location].red - gRedRover.weap->red) < 0.00001)
			{
				killer = &gRedRover;
			}
			else if (fabs(gTiles[location].red - gPinkBubbleGum.weap->red) < 0.00001)
			{
				killer = &gPinkBubbleGum;
			}
			else if (fabs(gTiles[location].red - gBlueLightning.weap->red) < 0.00001)
			{
				killer = &gBlueLightning;
			}
			else if (fabs(gTiles[location].red - gGreenTree.weap->red) < 0.00001)
			{
				killer = &gGreenTree;
			}
			
			if (killer)
			{
				(killer->kills)++;
				
				if (gNetworkConnection && gNetworkConnection->type == NETWORK_SERVER_TYPE)
				{
					sendToClients(0, "ck%i %i", IDOfCharacter(killer), killer->kills);
				}
			}
		}
		
		player->z -= CHARACTER_FALLING_SPEED * timeDelta;
		player->recovery_timer = 1;
		
		if (gNetworkConnection && gNetworkConnection->type == NETWORK_SERVER_TYPE)
		{
			sendToClients(0, "mo%i %f %f %f %i", IDOfCharacter(player), player->x, player->y, player->z, player->direction);
		}
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
			
			if (gNetworkConnection && gNetworkConnection->type == NETWORK_SERVER_TYPE)
			{
				// tell clients we spawned
				sendToClients(0, "sp%i %f %f", IDOfCharacter(player), player->x, player->y);
			}
			
			player->direction = player->backup_direction;
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
	gSecondTimer = 0.0;
	gLastSecond = 0.0;
	gTimeElapsedAccumulator = 0.0;
	
	gLayerColorIndex = 0;
	gLayerDeathIndex = 0;
	gLayerTwoColorIndex = 0;
	gLayerTwoDeathIndex = 0;
	
	gFirstLayerAnimationTimer = 0;
	gSecondLayerAnimationTimer = 0;
	
	// stop weapon animation and drawing
	gRedRover.weap->drawingState = SDL_FALSE;
	gRedRover.weap->animationState = SDL_FALSE;
	
	gGreenTree.weap->drawingState = SDL_FALSE;
	gGreenTree.weap->animationState = SDL_FALSE;
	
	gPinkBubbleGum.weap->drawingState = SDL_FALSE;
	gPinkBubbleGum.weap->animationState = SDL_FALSE;
	
	gBlueLightning.weap->drawingState = SDL_FALSE;
	gBlueLightning.weap->animationState = SDL_FALSE;
	
	gStatsTimer = 0;
	gCurrentWinner = 0;
}
