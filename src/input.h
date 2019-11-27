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

#include "characters.h"
#include "gamepad.h"
#include "sdl.h"

/* Keyboard input structure used for a character*/
typedef struct
{
	Character *character;
	
	// Is the weapon button being held down?
	bool weap;
	
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
	
	// right, left, up, down, weapon ids - their assigned keyboard keys.
	unsigned int r_id;
	unsigned int l_id;
	unsigned int u_id;
	unsigned int d_id;
	unsigned int weap_id;
	
	GamepadIndex gamepadIndex;
} Input;

extern Input gRedRoverInput;
extern Input gGreenTreeInput;
extern Input gPinkBubbleGumInput;
extern Input gBlueLightningInput;

void initInput(Input *input, int right, int left, int up, int down, int weapon);

Character *characterFromInput(Input *characterInput);

void performDownKeyAction(Input *input, SDL_Event *event);
void performUpKeyAction(Input *input, SDL_Event *event);

void performGamepadAction(Input *input, GamepadEvent *gamepadEvent);

void updateCharacterFromInput(Input *input);
void updateCharacterFromAnyInput(void);

bool joyButtonEventMatchesMovementFromInput(SDL_JoyButtonEvent *buttonEvent, Input *input);
