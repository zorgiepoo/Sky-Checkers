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

#include "scenery.h"
#include "weapon.h"
#include "math_3d.h"
#include "renderer.h"

#define CHARACTER_TERMINATING_Z -70.0f

#define NO_DIRECTION 0
#define RIGHT 1
#define LEFT 2
#define UP 3
#define DOWN 4

#define NO_CHARACTER 0
#define RED_ROVER 1
#define GREEN_TREE 2
#define BLUE_LIGHTNING 3
#define PINK_BUBBLE_GUM 4
// added because of configureJoyStick()
#define WEAPON 5

#define CHARACTER_HUMAN_STATE 1
#define CHARACTER_AI_STATE 2

#define AI_EASY_MODE 5
#define AI_MEDIUM_MODE 3
#define AI_HARD_MODE 1

#define NETWORK_NO_STATE 0
#define NETWORK_PENDING_STATE 5
#define NETWORK_PLAYING_STATE 6

#define MAX_CONTROLLER_NAME_SIZE 20

// If you were to use a stop watch, the time it takes for a character to go from one end
// of the checkerboard to the other end (vertically) is ~3.50-3.60 seconds
#define INITIAL_CHARACTER_SPEED	4.51977f
#define INITIAL_RECOVERY_TIME_DELAY (71 * 0.0177)

#define CHARACTER_ALIVE_Z 2.0f
#define CHARACTER_IS_ALIVE(character) (fabsf((character)->z - CHARACTER_ALIVE_Z) < 0.001f)

extern int gAIMode;

extern int gNumberOfNetHumans;

typedef struct _Character
{
	/* x, y, z translate values for character */
	float x, y, z;
	
	/* Distance traveled, used for tutorial */
	float distance;
	
	/* Distance traveled without stopping, used for tutorial */
	float nonstopDistance;
	
	/* Number of times weapon was fired, used for tutorial */
	uint16_t numberOfFires;
	
	/* Colors of character faces */
	float red, green, blue, alpha;
	
	/* Direction character is currently going in */
	int direction;
	
	/* Discrepancy x, y values -- used for netcode */
	float xDiscrepancy;
	float yDiscrepancy;
	
	/* The number of movements we have consumed -- used to determine when we should correct a player's position for netcode */
	int movementConsumedCounter;
	
	/* Predicted direction character client uses for netcode */
	int predictedDirection;
	uint32_t predictedDirectionTime;
	
	/* Speed of character */
	float speed;
	
	/* The direction the character was last pointing towards */
	int pointing_direction;
	
	/* Character's current amount of lives. When this reaches zero, the character dies */
	int lives;
	
	/* Stats */
	int kills;
	int wins;
	
	/*
	 * Character's state:
	 * CHARACTER_HUMAN_STATE
	 * CHARACTER_AI_STATE
	 */
	int state;
	// used when playing networked games as a server
	int backup_state;
	
	/* Character's network state */
	int netState;
	char *netName;
	
	Weapon *weap;
	bool active;
	
	/*
	 * Each character holds data for animation purposes, including stuff with tiles.
	 * Note: Most or all of this data is not included to be accessed through the console
	 */
	
	// weapon animation timer
	double animation_timer;
	// character recovery timer
	int recovery_timer;
	// flag that tells us if the character 'colored' a tile
	bool coloredTiles;
	// flag to tell us if we need a tile location in order for us to know which tiles to destroy
	bool needTileLoc;
	// recovery time delay for the tiles
	double recovery_time_delay;
	// the character's location on the checkerboard
	int player_loc;
	// a destroyed tile location that we use to find the other destroyed tiles
	int destroyedTileIndex;
	
	/* AI */
	float move_timer;
	float fire_timer;
	float time_alive;
	
	char controllerName[MAX_CONTROLLER_NAME_SIZE];
} Character;

extern Character gRedRover;
extern Character gGreenTree;
extern Character gPinkBubbleGum;
extern Character gBlueLightning;

void initCharacters(void);
void resetCharacterWins(void);
void loadCharacter(Character *character);

void restoreAllBackupStates(void);
int offlineCharacterState(Character *character);

void loadCharacterTextures(Renderer *renderer);

void buildCharacterModels(Renderer *renderer);

void drawCharacterIcons(Renderer *renderer, const mat4_t *translations);

void drawCharacters(Renderer *renderer, RendererOptions options);

void drawAllCharacterInfo(Renderer *renderer, const mat4_t *iconTranslations, bool displayControllerName);

void getOtherCharacters(Character *characterA, Character **characterB, Character **characterC, Character **characterD);

int IDOfCharacter(Character *character);
Character *getCharacter(int ID);

void spawnCharacter(Character *character);

void moveCharacter(Character *character, double timeDelta, bool pausedState);

void turnCharacter(Character *character, int direction);

void fireCharacterWeapon(Character *character);
void prepareFiringCharacterWeapon(Character *character, float x, float y, int direction, float compensation);
