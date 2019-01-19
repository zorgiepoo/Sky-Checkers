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

#define JOY_NONE	8001
#define JOY_UP		8002
#define JOY_RIGHT	8003
#define JOY_DOWN	8004
#define JOY_LEFT	8005
#define JOY_AXIS_NONE	100
#define JOY_INVALID_ID	-1

#define MAX_JOY_DESCRIPTION_BUFFER_LENGTH 128

// This dictates what the analog deadzone is. Max is 32767
#define VALID_ANALOG_MAGNITUDE 8100

/* Keyboard input structure used for a character*/
typedef struct
{
	Character *character;
	
	// Is the weapon button being held down?
	SDL_bool weap;
	
	// This is a bitmask for a lack of a better name.
	// Normally zero which means we pick the direction with latest timestamp
	// If non-zero, one or more of the directional inputs was triggered via a joystick analog type input
	// ..which means we pick the direction with the smallest timestamp
	uint8_t priority;
	
	// The time a particular direction started to be held down
	// Zero if they're not held down
	uint32_t right_ticks;
	uint32_t left_ticks;
	uint32_t up_ticks;
	uint32_t down_ticks;
	
	// right, left, up, down ids - their assigned keyboard keys.
	unsigned int r_id;
	unsigned int l_id;
	unsigned int u_id;
	unsigned int d_id;
	// and the weapon id.
	unsigned int weap_id;
	
	// joy stick ids.
	int rjs_id;
	int ljs_id;
	int ujs_id;
	int djs_id;
	int weapjs_id;
	
	// joy stick axis ids (x, y)
	unsigned int rjs_axis_id;
	unsigned int ljs_axis_id;
	unsigned int ujs_axis_id;
	unsigned int djs_axis_id;
	unsigned int weapjs_axis_id;
	
	// joystick instance ids which tell us what joystick is being triggered
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

void performDownAction(Input *input, SDL_Event *event);
void performUpAction(Input *input, SDL_Event *event);

void updateCharacterFromInput(Input *input);
void updateCharacterFromAnyInput(void);
