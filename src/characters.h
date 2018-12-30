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
#include "scenery.h"
#include "weapon.h"
#include "math_3d.h"
#include "renderer.h"

#define CHARACTER_TERMINATING_Z -70.0f

extern const int NO_DIRECTION;
extern const int RIGHT;
extern const int LEFT;
extern const int UP;
extern const int DOWN;

extern const int NO_CHARACTER;
extern const int RED_ROVER;
extern const int GREEN_TREE;
extern const int BLUE_LIGHTNING;
extern const int PINK_BUBBLE_GUM;
// added because of configureJoyStick()
extern const int WEAPON;

extern const int CHARACTER_HUMAN_STATE;
extern const int CHARACTER_AI_STATE;

extern const int AI_EASY_MODE;
extern const int AI_MEDIUM_MODE;
extern const int AI_HARD_MODE;

extern const int NETWORK_NO_STATE;
extern const int NETWORK_PENDING_STATE;
extern const int NETWORK_PLAYING_STATE;


// If you were to use a stop watch, the time it takes for a character to go from one end
// of the checkerboard to the other end (vertically) is ~3.50-3.60 seconds
#define INITIAL_CHARACTER_SPEED	4.51977f

#define CHARACTER_ALIVE_Z 2.0f
#define CHARACTER_IS_ALIVE(character) (fabsf((character)->z - CHARACTER_ALIVE_Z) < 0.001f)

extern int gAIMode;
extern int gAINetMode;

extern int gNumberOfNetHumans;

typedef struct _Character
{
	/* x, y, z translate values for character */
	float x, y, z;
	
	/* Colors of character faces */
	float red, green, blue;
	
	/* Direction character is currently going in */
	int direction;
	
	/* The last direction the character is currently going in - used for netcode */
	int last_direction;
	
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
	SDL_bool active;
	
	/*
	 * Each character holds data for animation purposes, including stuff with tiles.
	 * Note: Most or all of this data is not included to be accessed through the console
	 */
	
	// weapon animation timer
	int animation_timer;
	// character recovery timer
	int recovery_timer;
	// flag that tells us if the character 'colored' a tile
	SDL_bool coloredTiles;
	// flag to tell us if we need a tile location in order for us to know which tiles to destroy
	SDL_bool needTileLoc;
	// recovery time delay for the tiles
	int recovery_time_delay;
	// the character's location on the checkerboard
	int player_loc;
	// pointer to a destroyed tile that we use to find the other destroyed tiles
	struct _tile *destroyed_tile;
	
	/* AI */
	int ai_timer;
	int time_alive;
} Character;

extern Character gRedRover;
extern Character gGreenTree;
extern Character gPinkBubbleGum;
extern Character gBlueLightning;

void initCharacters(void);
void loadCharacter(Character *character);

void restoreAllBackupStates(void);
int offlineCharacterState(Character *character);

void loadCharacterTextures(Renderer *renderer);

void buildCharacterModels(Renderer *renderer);

void drawCharacterIcons(Renderer *renderer, const mat4_t *translations);

void drawCharacters(Renderer *renderer);

void drawCharacterLives(Renderer *renderer);

void getOtherCharacters(Character *characterA, Character **characterB, Character **characterC, Character **characterD);

int IDOfCharacter(Character *character);
Character *getCharacter(int ID);

void spawnCharacter(Character *character);

void moveCharacter(Character *character, double timeDelta);

void turnCharacter(Character *character, int direction);

void fireCharacterWeapon(Character *character);
void prepareFiringCharacterWeapon(Character *character, float x, float y);
