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

static void turnOffDirectionsExcept(Input *input, int direction);

void initInput(Input *input, int right, int left, int up, int down, int weapon)
{
	input->right_ticks = 0;
	input->left_ticks = 0;
	input->up_ticks = 0;
	input->down_ticks = 0;
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

void performDownAction(Input *input, SDL_Event *event)
{
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
		
		else if (input->right_ticks == 0 && event->key.keysym.scancode == input->r_id)
		{
			input->right_ticks = event->key.timestamp;
		}
	
		else if (input->left_ticks == 0 && event->key.keysym.scancode == input->l_id)
		{
			input->left_ticks = event->key.timestamp;
		}
	
		else if (input->up_ticks == 0 && event->key.keysym.scancode == input->u_id)
		{
			input->up_ticks = event->key.timestamp;
		}
	
		else if (input->down_ticks == 0 && event->key.keysym.scancode == input->d_id)
		{
			input->down_ticks = event->key.timestamp;
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
					input->up_ticks = event->key.timestamp;
				else
					input->up_ticks = 0;
			}
			else if (input->ujs_id < -32000)
			{
				if (event->jaxis.value < -32000)
					input->up_ticks = event->key.timestamp;
				else
					input->up_ticks = 0;
			}
			else
			{
				input->up_ticks = 0;
			}
		}
		
		if (input->joy_down_id == event->jaxis.which && input->djs_axis_id == event->jaxis.axis)
		{
			if (input->djs_id > 32000)
			{
				if (event->jaxis.value > 32000)
					input->down_ticks = event->key.timestamp;
				else
					input->down_ticks = 0;
			}
			else if (input->djs_id < -32000)
			{
				if (event->jaxis.value < -32000)
					input->down_ticks = event->key.timestamp;
				else
					input->down_ticks = 0;
			}
			else
			{
				input->down_ticks = 0;
			}
		}
		
		if (input->joy_right_id == event->jaxis.which && input->rjs_axis_id == event->jaxis.axis)
		{
			if (input->rjs_id > 32000)
			{
				if (event->jaxis.value > 32000)
					input->right_ticks = event->key.timestamp;
				else
					input->right_ticks = 0;
			}
			else if (input->rjs_id < -32000)
			{
				if (event->jaxis.value < -32000)
					input->right_ticks = event->key.timestamp;
				else
					input->right_ticks = 0;
			}
			else
			{
				input->right_ticks = 0;
			}
		}
		
		if (input->joy_left_id == event->jaxis.which && input->ljs_axis_id == event->jaxis.axis)
		{
			if (input->ljs_id > 32000)
			{
				if (event->jaxis.value > 32000)
					input->left_ticks = event->key.timestamp;
				else
					input->left_ticks = 0;
			}
			else if (input->ljs_id < -32000)
			{
				if (event->jaxis.value < -32000)
					input->left_ticks = event->key.timestamp;
				else
					input->left_ticks = 0;
			}
			else
			{
				input->left_ticks = 0;
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
				 input->right_ticks == 0 && input->left_ticks == 0 && input->down_ticks == 0)
		{
			input->up_ticks = event->key.timestamp;
		}
		
		else if (event->jbutton.which == input->joy_down_id && event->jbutton.button == input->djs_id && input->djs_axis_id == JOY_AXIS_NONE &&
				 input->right_ticks == 0 && input->left_ticks == 0 && input->up_ticks == 0)
		{
			input->down_ticks = event->key.timestamp;
		}
		
		else if (event->jbutton.which == input->joy_left_id && event->jbutton.button == input->ljs_id && input->ljs_axis_id == JOY_AXIS_NONE &&
				 input->right_ticks == 0 && input->down_ticks == 0 && input->up_ticks == 0)
		{
			input->left_ticks = event->key.timestamp;
		}
		
		else if (event->jbutton.which == input->joy_right_id && event->jbutton.button == input->rjs_id && input->rjs_axis_id == JOY_AXIS_NONE &&
				 input->left_ticks == 0 && input->down_ticks == 0 && input->up_ticks == 0)
		{
			input->right_ticks = event->key.timestamp;
		}
	}
}

void performUpAction(Input *input, SDL_Event *event)
{
	// If the user releases the key, we turn off the character's direction value.
	
	if (event->type == SDL_KEYUP)
	{
		if (input->right_ticks == 0 && input->left_ticks == 0 && input->down_ticks == 0 && input->up_ticks == 0 && !input->weap)
			return;
	
		if (event->key.keysym.scancode == input->r_id)
		{
			input->right_ticks = 0;
		}
	
		else if (event->key.keysym.scancode == input->l_id)
		{
			input->left_ticks = 0;
		}
	
		else if (event->key.keysym.scancode == input->u_id)
		{
			input->up_ticks = 0;
		}
	
		else if (event->key.keysym.scancode == input->d_id)
		{
			input->down_ticks = 0;
		}
		
		else if (event->key.keysym.scancode == input->weap_id)
		{
			if (gGameHasStarted)
			{
				prepareFiringCharacterWeapon(input->character, input->character->x, input->character->y, 0.0f);
			}
			input->weap = SDL_FALSE;
		}
	}
	else if (event->type == SDL_JOYBUTTONUP)
	{
		if (input->up_ticks != 0 && event->jbutton.which == input->joy_up_id && event->jbutton.button == input->ujs_id && input->ujs_axis_id == JOY_AXIS_NONE)
		{
			input->up_ticks = 0;
		}
		
		else if (input->down_ticks != 0 && event->jbutton.which == input->joy_down_id && event->jbutton.button == input->djs_id && input->djs_axis_id == JOY_AXIS_NONE)
		{
			input->down_ticks = 0;
		}
		
		else if (input->left_ticks != 0 && event->jbutton.which == input->joy_left_id && event->jbutton.button == input->ljs_id && input->ljs_axis_id == JOY_AXIS_NONE)
		{
			input->left_ticks = 0;
		}
		
		else if (input->right_ticks != 0 && event->jbutton.which == input->joy_right_id && event->jbutton.button == input->rjs_id && input->rjs_axis_id == JOY_AXIS_NONE)
		{
			input->right_ticks = 0;
		}
		
		else if (input->weap && event->jbutton.which == input->joy_weap_id && event->jbutton.button == input->weapjs_id && input->weapjs_axis_id == JOY_AXIS_NONE &&
				 !input->character->weap->animationState)
		{
			if (gGameHasStarted)
			{
				prepareFiringCharacterWeapon(input->character, input->character->x, input->character->y, 0.0f);
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
				prepareFiringCharacterWeapon(input->character, input->character->x, input->character->y, 0.0f);
			}
			input->weap = SDL_FALSE;
		}
	}
}

static uint32_t maxValue(uint32_t value, uint32_t value2)
{
	return (value > value2) ? value : value2;
}

static int bestDirectionFromInputTicks(uint32_t right_ticks, uint32_t left_ticks, uint32_t up_ticks, uint32_t down_ticks)
{
	uint32_t scores[] = {right_ticks, left_ticks, up_ticks, down_ticks};
	uint32_t maxScore = 0;
	uint8_t maxScoreIndex = 0;
	for (uint8_t scoreIndex = 0; scoreIndex < sizeof(scores) / sizeof(scores[0]); scoreIndex++)
	{
		if (scores[scoreIndex] > maxScore)
		{
			maxScore = scores[scoreIndex];
			maxScoreIndex = scoreIndex;
		}
	}
	
	if (maxScore == 0)
	{
		return NO_DIRECTION;
	}
	else
	{
		return maxScoreIndex + 1;
	}
}

void updateCharacterFromAnyInput(void)
{
	if (!gNetworkConnection || !gNetworkConnection->character)
	{
		return;
	}
	
	Character *character = gNetworkConnection->character;
	
	if (gNetworkConnection->type == NETWORK_SERVER_TYPE && !character->active)
	{
		return;
	}
	
	// Any input will move the character if it's a network game
	
	uint32_t rightTicks = maxValue(maxValue(maxValue(gRedRoverInput.right_ticks, gPinkBubbleGumInput.right_ticks), gGreenTreeInput.right_ticks), gBlueLightningInput.right_ticks);
	
	uint32_t leftTicks = maxValue(maxValue(maxValue(gRedRoverInput.left_ticks, gPinkBubbleGumInput.left_ticks), gGreenTreeInput.left_ticks), gBlueLightningInput.left_ticks);
	
	uint32_t upTicks = maxValue(maxValue(maxValue(gRedRoverInput.up_ticks, gPinkBubbleGumInput.up_ticks), gGreenTreeInput.up_ticks), gBlueLightningInput.up_ticks);
	
	uint32_t downTicks = maxValue(maxValue(maxValue(gRedRoverInput.down_ticks, gPinkBubbleGumInput.down_ticks), gGreenTreeInput.down_ticks), gBlueLightningInput.down_ticks);
	
	int newDirection = bestDirectionFromInputTicks(rightTicks, leftTicks, upTicks, downTicks);
	
	if (gNetworkConnection->type == NETWORK_CLIENT_TYPE)
	{
		if (newDirection != character->direction && character->active)
		{
			GameMessage message;
			message.type = MOVEMENT_REQUEST_MESSAGE_TYPE;
			message.movementRequest.direction = newDirection;

			setPredictedDirection(character, newDirection);

			sendToServer(message);
		}
	}
	else
	{
		character->direction = newDirection;
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
	
	input->character->direction = bestDirectionFromInputTicks(input->right_ticks, input->left_ticks, input->up_ticks, input->down_ticks);
}
