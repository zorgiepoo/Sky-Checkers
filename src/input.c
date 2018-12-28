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

#include "input.h"
#include "network.h"

const unsigned JOY_NONE =		8001;
const unsigned JOY_UP =			8002;
const unsigned JOY_RIGHT =		8003;
const unsigned JOY_DOWN =		8004;
const unsigned JOY_LEFT =		8005;
const unsigned JOY_AXIS_NONE =	100;
const unsigned JOY_INVALID_ID =	-1;

Input gRedRoverInput;
Input gGreenTreeInput;
Input gPinkBubbleGumInput;
Input gBlueLightningInput;

void initInput(Input *input, int right, int left, int up, int down, int weapon)
{
	input->right = SDL_FALSE;
	input->left = SDL_FALSE;
	input->up = SDL_FALSE;
	input->down = SDL_FALSE;
	input->weap = SDL_FALSE;
	
	input->u_id = 0;
	input->d_id = 0;
	input->l_id = 0;
	input->r_id = 0;
	input->weap_id = 0;
	
	input->ujs_id = JOY_NONE;
	input->djs_id = JOY_NONE;
	input->ljs_id = JOY_NONE;
	input->rjs_id = JOY_NONE;
	input->weapjs_id = JOY_NONE;
	
	input->rjs_axis_id = JOY_AXIS_NONE;
	input->ljs_axis_id = JOY_AXIS_NONE;
	input->ujs_axis_id = JOY_AXIS_NONE;
	input->djs_axis_id = JOY_AXIS_NONE;
	input->weapjs_axis_id = JOY_AXIS_NONE;
	
	// allocate some memory for the descriptions.
	input->joy_right = calloc(MAX_JOY_DESCRIPTION_BUFFER_LENGTH, 1);
	input->joy_left = calloc(MAX_JOY_DESCRIPTION_BUFFER_LENGTH, 1);
	input->joy_up = calloc(MAX_JOY_DESCRIPTION_BUFFER_LENGTH, 1);
	input->joy_down = calloc(MAX_JOY_DESCRIPTION_BUFFER_LENGTH, 1);
	input->joy_weap = calloc(MAX_JOY_DESCRIPTION_BUFFER_LENGTH, 1);
	
	sprintf(input->joy_right, "None");
	sprintf(input->joy_left, "None");
	sprintf(input->joy_down, "None");
	sprintf(input->joy_up, "None");
	sprintf(input->joy_weap, "None");
	
	input->r_id = right;
	input->l_id = left;
	input->u_id = up;
	input->d_id = down;
	input->weap_id = weapon;
}

/* Returns the Character that corresponds to the characterInput. This is more reliable than using input->character when there's a network connection alive because the character pointer variable may be different */
Character *characterFromInput(Input *characterInput)
{
	if (!gNetworkConnection)
	{
		return characterInput->character;
	}
	
	if (characterInput == &gPinkBubbleGumInput)
		return &gPinkBubbleGum;
	
	if (characterInput == &gRedRoverInput)
		return &gRedRover;
	
	if (characterInput == &gGreenTreeInput)
		return &gGreenTree;
	
	if (characterInput == &gBlueLightningInput)
		return &gBlueLightning;
	
	return NULL;
}

void performDownAction(Input *input, SDL_Window *window, SDL_Event *event)
{
	// this is a check for joysticks, mainly
	if ((SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) == 0)
	{
		return;
	}
	// character needs to be living
	if (!input->character->lives)
		return;
	
	// character needs to be on the checkerboard.
	if (input->character->z != 2.0)
		return;
	
	// check if the input is pressed and set the character's direction value accordingly
	
	if (event->type == SDL_KEYDOWN)
	{
		if (event->key.keysym.scancode == input->weap_id && !input->character->weap->animationState)
		{
			if (gGameHasStarted)
			{
				input->weap = SDL_TRUE;
			}
		}
		
		else if (!input->right && event->key.keysym.scancode == input->r_id)
		{
			input->right = SDL_TRUE;
			turnOffDirectionsExcept(input, RIGHT);
		}
	
		else if (!input->left && event->key.keysym.scancode == input->l_id)
		{
			input->left = SDL_TRUE;
			turnOffDirectionsExcept(input, LEFT);
		}
	
		else if (!input->up && event->key.keysym.scancode == input->u_id)
		{
			input->up = SDL_TRUE;
			turnOffDirectionsExcept(input, UP);
		}
	
		else if (!input->down && event->key.keysym.scancode == input->d_id)
		{
			input->down = SDL_TRUE;
			turnOffDirectionsExcept(input, DOWN);
		}
	}
	
	// joystick input. see if it's pressed or released and set direction value accordingly
	
	else if (event->type == SDL_JOYAXISMOTION)
	{
		
		if (input->joy_up_id == event->jaxis.which && input->ujs_axis_id == event->jaxis.axis)
		{
			if (input->ujs_id > 32000)
			{
				if (event->jaxis.value > 32000)
					input->up = SDL_TRUE;
				else
					input->up = SDL_FALSE;
			}
			else if (input->ujs_id < -32000)
			{
				if (event->jaxis.value < -32000)
					input->up = SDL_TRUE;
				else
					input->up = SDL_FALSE;
			}
			
			if (input->up)
			{
				turnOffDirectionsExcept(input, UP);
			}
		}
		
		if (input->joy_down_id == event->jaxis.which && input->djs_axis_id == event->jaxis.axis)
		{
			if (input->djs_id > 32000)
			{
				if (event->jaxis.value > 32000)
					input->down = SDL_TRUE;
				else
					input->down = SDL_FALSE;
			}
			else if (input->djs_id < -32000)
			{
				if (event->jaxis.value < -32000)
					input->down = SDL_TRUE;
				else
					input->down = SDL_FALSE;
			}
			
			if (input->down)
			{
				turnOffDirectionsExcept(input, DOWN);
			}
		}
		
		if (input->joy_right_id == event->jaxis.which && input->rjs_axis_id == event->jaxis.axis)
		{
			if (input->rjs_id > 32000)
			{
				if (event->jaxis.value > 32000)
					input->right = SDL_TRUE;
				else
					input->right = SDL_FALSE;
			}
			else if (input->rjs_id < -32000)
			{
				if (event->jaxis.value < -32000)
					input->right = SDL_TRUE;
				else
					input->right = SDL_FALSE;
			}
			
			if (input->right)
			{
				turnOffDirectionsExcept(input, RIGHT);
			}
		}
		
		if (input->joy_left_id == event->jaxis.which && input->ljs_axis_id == event->jaxis.axis)
		{
			if (input->ljs_id > 32000)
			{
				if (event->jaxis.value > 32000)
					input->left = SDL_TRUE;
				else
					input->left = SDL_FALSE;
			}
			else if (input->ljs_id < -32000)
			{
				if (event->jaxis.value < -32000)
					input->left = SDL_TRUE;
				else
					input->left = SDL_FALSE;
			}
			
			if (input->left)
			{
				turnOffDirectionsExcept(input, LEFT);
			}
		}
		
		if (!input->character->weap->animationState && event->jaxis.axis == input->weapjs_axis_id && event->jaxis.which == input->joy_weap_id)
		{
			
			if (input->weapjs_id > 32000)
			{
				if (event->jaxis.value > 32000)
				{
					if (gGameHasStarted)
						input->weap = SDL_TRUE;
				}
			}
			else if (input->weapjs_id < -32000)
			{
				if (event->jaxis.value < -32000)
				{
					if (gGameHasStarted)
						input->weap = SDL_TRUE;
				}
			}
		}
	}
	
	else if (event->type == SDL_JOYBUTTONDOWN)
	{
		
		if (event->jbutton.which == input->joy_weap_id && event->jbutton.button == input->weapjs_id && input->weapjs_axis_id == JOY_AXIS_NONE &&
			!input->character->weap->animationState && !input->weap)
		{
			
			if (gGameHasStarted)
				input->weap = SDL_TRUE;
		}
		
		else if (event->jbutton.which == input->joy_up_id && event->jbutton.button == input->ujs_id && input->ujs_axis_id == JOY_AXIS_NONE &&
				 !input->right && !input->left && !input->down)
		{
			input->up = SDL_TRUE;
			turnOffDirectionsExcept(input, UP);
		}
		
		else if (event->jbutton.which == input->joy_down_id && event->jbutton.button == input->djs_id && input->djs_axis_id == JOY_AXIS_NONE &&
				 !input->right && !input->left && !input->up)
		{
			input->down = SDL_TRUE;
			turnOffDirectionsExcept(input, DOWN);
		}
		
		else if (event->jbutton.which == input->joy_left_id && event->jbutton.button == input->ljs_id && input->ljs_axis_id == JOY_AXIS_NONE &&
				 !input->right && !input->down && !input->up)
		{
			input->left = SDL_TRUE;
			turnOffDirectionsExcept(input, LEFT);
		}
		
		else if (event->jbutton.which == input->joy_right_id && event->jbutton.button == input->rjs_id && input->rjs_axis_id == JOY_AXIS_NONE &&
				 !input->left && !input->down && !input->up)
		{
			input->right = SDL_TRUE;
			turnOffDirectionsExcept(input, RIGHT);
		}
	}
}

void performUpAction(Input *input, SDL_Window *window, SDL_Event *event)
{
	if ((SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) == 0)
	{
		return;
	}
	
	// If the user releases the key, we turn off the character's direction value.
	
	if (event->type == SDL_KEYUP)
	{
		if (!input->right && !input->left && !input->down && !input->up && !input->weap)
			return;
	
		if (event->key.keysym.scancode == input->r_id)
		{
			input->right = SDL_FALSE;
		}
	
		else if (event->key.keysym.scancode == input->l_id)
		{
			input->left = SDL_FALSE;
		}
	
		else if (event->key.keysym.scancode == input->u_id)
		{
			input->up = SDL_FALSE;
		}
	
		else if (event->key.keysym.scancode == input->d_id)
		{
			input->down = SDL_FALSE;
		}
		
		else if (event->key.keysym.scancode == input->weap_id)
		{
			if (gGameHasStarted)
			{
				prepareFiringCharacterWeapon(input->character, input->character->x, input->character->y);
			}
			input->weap = SDL_FALSE;
		}
	}
	else if (event->type == SDL_JOYBUTTONUP)
	{
		if (input->up && event->jbutton.which == input->joy_up_id && event->jbutton.button == input->ujs_id && input->ujs_axis_id == JOY_AXIS_NONE)
		{
			input->up = SDL_FALSE;
		}
		
		else if (input->down && event->jbutton.which == input->joy_down_id && event->jbutton.button == input->djs_id && input->djs_axis_id == JOY_AXIS_NONE)
		{
			input->down = SDL_FALSE;
		}
		
		else if (input->left && event->jbutton.which == input->joy_left_id && event->jbutton.button == input->ljs_id && input->ljs_axis_id == JOY_AXIS_NONE)
		{
			input->left = SDL_FALSE;
		}
		
		else if (input->right && event->jbutton.which == input->joy_right_id && event->jbutton.button == input->rjs_id && input->rjs_axis_id == JOY_AXIS_NONE)
		{
			input->right = SDL_FALSE;
		}
		
		else if (input->weap && event->jbutton.which == input->joy_weap_id && event->jbutton.button == input->weapjs_id && input->weapjs_axis_id == JOY_AXIS_NONE &&
				 !input->character->weap->animationState)
		{
			if (gGameHasStarted)
			{
				prepareFiringCharacterWeapon(input->character, input->character->x, input->character->y);
			}
			input->weap = SDL_FALSE;
		}
	}
	else if (event->type == SDL_JOYAXISMOTION)
	{
		/* Other JOYAXISMOTION UPs are dealt with in performDownAction() (because it was easier to deal with there) */
		
		if (input->weap && !input->character->weap->animationState && input->weapjs_axis_id != JOY_AXIS_NONE)
		{
			if (gGameHasStarted)
			{
				prepareFiringCharacterWeapon(input->character, input->character->x, input->character->y);
			}
			input->weap = SDL_FALSE;
		}
	}
}

void updateCharacterFromAnyInput(void)
{
	if (!gNetworkConnection || !gNetworkConnection->character)
	{
		return;
	}
	
	Character *character = gNetworkConnection->character;
	
	if (!character->active)
	{
		return;
	}
	
	// Any input will move the character if it's a network game, however, only one input can be active at a time
	
	SDL_bool inputRedRoverState = gRedRoverInput.right || gRedRoverInput.left || gRedRoverInput.up || gRedRoverInput.down;
	SDL_bool inputPinkBubbleGumState = gPinkBubbleGumInput.right || gPinkBubbleGumInput.left || gPinkBubbleGumInput.up || gPinkBubbleGumInput.down;
	SDL_bool inputGreenTreeState = gGreenTreeInput.right || gGreenTreeInput.left || gGreenTreeInput.up || gGreenTreeInput.down;
	SDL_bool inputBlueLightningState = gBlueLightningInput.right || gBlueLightningInput.left || gBlueLightningInput.up || gBlueLightningInput.down;
	
	if (inputRedRoverState + inputPinkBubbleGumState + inputGreenTreeState + inputBlueLightningState > 1)
	{
		character->direction = NO_DIRECTION;
	}
	else if (gRedRoverInput.right || gPinkBubbleGumInput.right || gGreenTreeInput.right || gBlueLightningInput.right)
	{
		character->direction = RIGHT;
	}
	else if (gRedRoverInput.left || gPinkBubbleGumInput.left || gGreenTreeInput.left || gBlueLightningInput.left)
	{
		character->direction = LEFT;
	}
	else if (gRedRoverInput.down || gPinkBubbleGumInput.down || gGreenTreeInput.down || gBlueLightningInput.down)
	{
		character->direction = DOWN;
	}
	else if (gRedRoverInput.up || gPinkBubbleGumInput.up || gGreenTreeInput.up || gBlueLightningInput.up)
	{
		character->direction = UP;
	}
	else
	{
		character->direction = NO_DIRECTION;
	}
}

void updateCharacterFromInput(Input *input)
{
	if (gNetworkConnection || (input->character->state != CHARACTER_HUMAN_STATE))
	{
		return;
	}
	
	if (!input->character->active)
	{
		return;
	}
	
	if (input->right)
	{
		input->character->direction = RIGHT;
	}
	else if (input->left)
	{
		input->character->direction = LEFT;
	}
	else if (input->up)
	{
		input->character->direction = UP;
	}
	else if (input->down)
	{
		input->character->direction = DOWN;
	}
	else
	{
		input->character->direction = NO_DIRECTION;
	}
}

void turnOffDirectionsExcept(Input *input, int direction)
{
	if (direction != RIGHT)
	{
		input->right = SDL_FALSE;
	}
	
	if (direction != LEFT)
	{
		input->left = SDL_FALSE;
	}
	
	if (direction != UP)
	{
		input->up = SDL_FALSE;
	}
	
	if (direction != DOWN)
	{
		input->down = SDL_FALSE;
	}
}

void turnInputOff(Input *input)
{
	input->weap = SDL_FALSE;
	input->right = SDL_FALSE;
	input->left = SDL_FALSE;
	input->down = SDL_FALSE;
	input->up = SDL_FALSE;
}
