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

// in milliseconds
static const uint32_t TIMER_INTERVAL =				16;

static const float TILE_FALLING_SPEED =			1.8f;

// in seconds
static const int STATS_WILL_APPEAR =			4;

// We'll need 18 tiles at max (starting index is 1, not 0)
static int gTilesLayer[29];

static int gLayerColorIndex =					1;
static int gLayerDeathIndex =					1;
static int gLayerTwoColorIndex =				1;
static int gLayerTwoDeathIndex =				1;

static float gSecondTimer =						0.0f;
static float gLastSecond =						0.0f;

static int gFirstLayerAnimationTimer =			0;
static int gSecondLayerAnimationTimer =			0;

static int gCurrentWinner =						0;
static int gStatsTimer =						0;

static SDL_TimerID gTimer;

/* Functions */

static Uint32 timedEvents(Uint32 interval, void *param);

static void colorTile(Tile *tile, Weapon *weap);

static void animateWeapAndTiles(SDL_Window *window, Character *player);

static void firstTileLayerAnimation(SDL_Window *window, int beginAnimating, int endAnimating);
static void secondTileLayerAnimation(SDL_Window *window, int beginAnimating, int endAnimating);

static void loadFirstTileAnimationLayer(void);
static void loadSecondTileAnimationLayer(void);

static void collapseTiles(void);
static void recoverDestroyedTiles(void);

static void killCharacter(Input *characterInput);
static void recoverCharacter(Character *player);

static Uint32 timedEvents(Uint32 interval, void *param)
{
	SDL_Event event;
	
	event.type = SDL_USEREVENT;
    event.user.code = 1;
    event.user.data1 = 0;
    event.user.data2 = 0;
	
	SDL_Window *window = (SDL_Window *)param;
	
	gSecondTimer += TIMER_INTERVAL / 1000.0f;
	
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
	
	// Move characters
	moveCharacterFromInput(&gRedRoverInput);
	moveCharacterFromInput(&gGreenTreeInput);
	moveCharacterFromInput(&gPinkBubbleGumInput);
	moveCharacterFromInput(&gBlueLightningInput);
	
	// Move AIs
	moveAI(&gRedRover, (int)gSecondTimer);
	moveAI(&gGreenTree, (int)gSecondTimer);
	moveAI(&gPinkBubbleGum, (int)gSecondTimer);
	moveAI(&gBlueLightning, (int)gSecondTimer);
	
	static const int BEGIN_TILE_LAYER_ANIMATION =	100;
	static const int END_TILE_LAYER_ANIMATION =	200;
	
	firstTileLayerAnimation(window, BEGIN_TILE_LAYER_ANIMATION, END_TILE_LAYER_ANIMATION);
	secondTileLayerAnimation(window, BEGIN_TILE_LAYER_ANIMATION, END_TILE_LAYER_ANIMATION);
	
	animateWeapAndTiles(window, &gRedRover);
	animateWeapAndTiles(window, &gGreenTree);
	animateWeapAndTiles(window, &gPinkBubbleGum);
	animateWeapAndTiles(window, &gBlueLightning);
	
	collapseTiles();
	recoverDestroyedTiles();
	
	killCharacter(&gRedRoverInput);
	killCharacter(&gGreenTreeInput);
	killCharacter(&gPinkBubbleGumInput);
	killCharacter(&gBlueLightningInput);
	
	recoverCharacter(&gRedRover);
	recoverCharacter(&gGreenTree);
	recoverCharacter(&gPinkBubbleGum);
	recoverCharacter(&gBlueLightning);
    
    SDL_PushEvent(&event);
	
	return interval;
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

static void animateWeapAndTiles(SDL_Window *window, Character *player)
{
	if (player->weap->animationState)
	{
		if (player->animation_timer == 0 && ((SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0) && gAudioEffectsFlag)
		{
			playShootingSound();
		}
		player->animation_timer++;
		
		static const float WEAPON_PROJECTILE_SPEED = 0.8f;
		
		if (player->weap->direction == RIGHT)
		{
			player->weap->x += WEAPON_PROJECTILE_SPEED;
		}
		else if (player->weap->direction == LEFT)
		{
			player->weap->x -= WEAPON_PROJECTILE_SPEED;
		}
		else if (player->weap->direction == UP)
		{
			player->weap->y += WEAPON_PROJECTILE_SPEED;
		}
		else if (player->weap->direction == DOWN)
		{
			player->weap->y -= WEAPON_PROJECTILE_SPEED;
		}
		
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
				player->destroyed_tile->z -= TILE_FALLING_SPEED;
				
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
static void firstTileLayerAnimation(SDL_Window *window, int beginAnimating, int endAnimating)
{
	// Color the tiles gray
	if (gLayerColorIndex != 0 && gFirstLayerAnimationTimer > beginAnimating)
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
		
		if (gLayerColorIndex == 29)
		{
			gLayerColorIndex = 0;
		}
	}
	
	// Make the tiles fall down
	if (gLayerDeathIndex != 0 && gFirstLayerAnimationTimer > endAnimating)
	{
		if (!gTiles[gTilesLayer[gLayerDeathIndex]].isDead)
		{
			gTiles[gTilesLayer[gLayerDeathIndex]].z -= TILE_FALLING_SPEED;
			gTiles[gTilesLayer[gLayerDeathIndex]].isDead = SDL_TRUE;
			
			if (((SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0) && gAudioEffectsFlag)
			{
				playTileFallingSound();
			}
		}
		
		gLayerDeathIndex++;
		
		if (gLayerDeathIndex == 29)
		{
			gLayerDeathIndex = 0;
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
static void secondTileLayerAnimation(SDL_Window *window, int beginAnimating, int endAnimating)
{
	// Color the tiles gray
	if (gLayerTwoColorIndex != 0 && gSecondLayerAnimationTimer > beginAnimating)
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
		
		if (gLayerTwoColorIndex == 21)
		{
			gLayerTwoColorIndex = 0;
		}
	}
	
	// Make the tiles fall down
	if (gLayerTwoDeathIndex != 0 && gSecondLayerAnimationTimer > endAnimating)
	{
		if (!gTiles[gTilesLayer[gLayerTwoDeathIndex]].isDead)
		{
			gTiles[gTilesLayer[gLayerTwoDeathIndex]].z -= TILE_FALLING_SPEED;
			gTiles[gTilesLayer[gLayerTwoDeathIndex]].isDead = SDL_TRUE;
			
			if (((SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0) && gAudioEffectsFlag)
			{
				playTileFallingSound();
			}
		}
		
		gLayerTwoDeathIndex++;
		
		if (gLayerTwoDeathIndex == 21)
		{
			gLayerTwoDeathIndex = 0;
			gSecondLayerAnimationTimer = -1;
		}
	}
	
	if (gSecondLayerAnimationTimer > 0 && gFirstLayerAnimationTimer == -1)
		gSecondLayerAnimationTimer++;
}

static void collapseTiles(void)
{
	int tileIndex;
	
	for (tileIndex = 1; tileIndex <= 64; tileIndex++)
	{
		if (gTiles[tileIndex].z <= -26.0)
		{
			gTiles[tileIndex].z -= TILE_FALLING_SPEED / 4;
		}
	}
}

/* To activate a recovery of a tile, set its recover_timer to a value greater than 0 */
static void recoverDestroyedTiles(void)
{
	int tileIndex;
	
	for (tileIndex = 1; tileIndex <= 64; tileIndex++)
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
			gTiles[tileIndex].z = -25.0;
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
			SDL_bool winnerIsReadyForDeath = (gTiles[getTileIndexLocation((int)winnerCharacter->x, (int)winnerCharacter->y)].z != -25.0 && winnerCharacter->z >= -70.0 && winnerCharacter->lives == 1);
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
static void killCharacter(Input *characterInput)
{
	// client's don't kill characters...
	// server tells them when to kill!
	if (gNetworkConnection && gNetworkConnection->type == NETWORK_CLIENT_TYPE)
	{
		return;
	}
	
	Character *player = characterFromInput(characterInput);
	
	int location = getTileIndexLocation((int)player->x, (int)player->y);
	
	/* If the tile the character is on isn't at its correct location, then the tile must be falling down, so we make the character fall down as well too */
	if (gTiles[location].z != -25.0 && player->z >= -70.0)
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
		
		static const float CHARACTER_FALLING_SPEED = 0.45f;
		
		player->z -= CHARACTER_FALLING_SPEED;
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
	gTilesLayer[1] = 57;
	gTilesLayer[2] = 58;
	gTilesLayer[3] = 59;
	gTilesLayer[4] = 60;
	gTilesLayer[5] = 61;
	gTilesLayer[6] = 62;
	gTilesLayer[7] = 63;
	gTilesLayer[8] = 64;
	
	gTilesLayer[9] = 56;
	gTilesLayer[10] = 48;
	gTilesLayer[11] = 40;
	gTilesLayer[12] = 32;
	gTilesLayer[13] = 24;
	gTilesLayer[14] = 16;
	gTilesLayer[15] = 8;
	
	gTilesLayer[16] = 7;
	gTilesLayer[17] = 6;
	gTilesLayer[18] = 5;
	gTilesLayer[19] = 4;
	gTilesLayer[20] = 3;
	gTilesLayer[21] = 2;
	gTilesLayer[22] = 1;
	
	gTilesLayer[23] = 9;
	gTilesLayer[24] = 17;
	gTilesLayer[25] = 25;
	gTilesLayer[26] = 33;
	gTilesLayer[27] = 41;
	gTilesLayer[28] = 49;
}

/* Second most outter tile layer */
static void loadSecondTileAnimationLayer(void)
{
	gTilesLayer[1] = 50;
	gTilesLayer[2] = 51;
	gTilesLayer[3] = 52;
	gTilesLayer[4] = 53;
	gTilesLayer[5] = 54;
	gTilesLayer[6] = 55;
	
	gTilesLayer[7] = 47;
	gTilesLayer[8] = 39;
	gTilesLayer[9] = 31;
	gTilesLayer[10] = 23;
	gTilesLayer[11] = 15;
	
	gTilesLayer[12] = 14;
	gTilesLayer[13] = 13;
	gTilesLayer[14] = 12;
	gTilesLayer[15] = 11;
	gTilesLayer[16] = 10;
	
	gTilesLayer[17] = 18;
	gTilesLayer[18] = 26;
	gTilesLayer[19] = 34;
	gTilesLayer[20] = 42;
}

SDL_bool startAnimation(SDL_Window *window)
{
	if (SDL_Init(SDL_INIT_TIMER) < 0)
	{
		zgPrint("Couldn't initialize SDL Timer: %e");
		return SDL_FALSE;
	}
	
	loadFirstTileAnimationLayer();
	
	gTimer = SDL_AddTimer(TIMER_INTERVAL, timedEvents, window);
	
	return SDL_TRUE;
}

void endAnimation(void)
{
	if (!SDL_RemoveTimer(gTimer))
		zgPrint("Removing timer failed");
	
	SDL_QuitSubSystem(SDL_INIT_TIMER);
	
	gSecondTimer = 0.0;
	gLastSecond = 0.0;
	
	gLayerColorIndex = 1;
	gLayerDeathIndex = 1;
	gLayerTwoColorIndex = 1;
	gLayerTwoDeathIndex = 1;
	
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
