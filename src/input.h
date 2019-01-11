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
#include "characters.h"

extern const unsigned JOY_NONE;
extern const unsigned JOY_UP;
extern const unsigned JOY_RIGHT;
extern const unsigned JOY_DOWN;
extern const unsigned JOY_LEFT;
extern const unsigned JOY_AXIS_NONE;
extern const unsigned JOY_INVALID_ID;

#define MAX_JOY_DESCRIPTION_BUFFER_LENGTH 128

/* Keyboard input structure used for a character*/
typedef struct
{
	Character *character;
	
	// Are the direction keys of the character held down or not?
	SDL_bool right;
	SDL_bool left;
	SDL_bool up;
	SDL_bool down;
	SDL_bool weap;
	
	// right, left, up, down ids - their assigned keyboard keys.
	unsigned r_id;
	unsigned l_id;
	unsigned u_id;
	unsigned d_id;
	// and the weapon id.
	unsigned weap_id;
	
	// joy stick ids.
	int rjs_id;
	int ljs_id;
	int ujs_id;
	int djs_id;
	int weapjs_id;
	
	// joy stick axis ids
	unsigned rjs_axis_id;
	unsigned ljs_axis_id;
	unsigned ujs_axis_id;
	unsigned djs_axis_id;
	unsigned weapjs_axis_id;
	
	// other joy stick ids. (Used for multiple amount of joysticks on the system and to indicate which joystick the input has)
	int joy_right_id;
	int joy_left_id;
	int joy_down_id;
	int joy_up_id;
	int joy_weap_id;
	
	// descriptions of joy actions
	char *joy_right;
	char *joy_left;
	char *joy_up;
	char *joy_down;
	char *joy_weap;
} Input;

extern Input gRedRoverInput;
extern Input gGreenTreeInput;
extern Input gPinkBubbleGumInput;
extern Input gBlueLightningInput;

void initInput(Input *input, int right, int left, int up, int down, int weapon);

Character *characterFromInput(Input *characterInput);

void performDownAction(Input *input, SDL_Window *window, SDL_Event *event);
void performUpAction(Input *input, SDL_Window *window, SDL_Event *event);

void updateCharacterFromInput(Input *input);
void updateCharacterFromAnyInput(void);
void turnOffDirectionsExcept(Input *input, int direction);
