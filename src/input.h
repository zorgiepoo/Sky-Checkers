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
#include "platforms.h"

#ifndef IOS_DEVICE
#include "keyboard.h"
#endif

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
	uint32_t r_id;
	uint32_t l_id;
	uint32_t u_id;
	uint32_t d_id;
	uint32_t weap_id;
	
	GamepadIndex gamepadIndex;
} Input;

extern Input gRedRoverInput;
extern Input gGreenTreeInput;
extern Input gPinkBubbleGumInput;
extern Input gBlueLightningInput;

void initInput(Input *input);

#ifndef IOS_DEVICE
void setInputKeys(Input *input, uint32_t right, uint32_t left, uint32_t up, uint32_t down, uint32_t weapon);
#endif

Character *characterFromInput(Input *characterInput);

#ifndef IOS_DEVICE
void performDownKeyAction(Input *input, ZGKeyboardEvent *event);
void performUpKeyAction(Input *input, ZGKeyboardEvent *event);
#endif

void performGamepadAction(Input *input, GamepadEvent *gamepadEvent);

void updateCharacterFromInput(Input *input);
void updateCharacterFromAnyInput(void);
