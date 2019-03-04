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
#include <stdint.h>
#include "network.h"
#include "collision.h"

Input gRedRoverInput;
Input gGreenTreeInput;
Input gPinkBubbleGumInput;
Input gBlueLightningInput;

void initInput(Input *input, int right, int left, int up, int down, int weapon)
{
	input->right_ticks = 0;
	input->left_ticks = 0;
	input->up_ticks = 0;
	input->down_ticks = 0;
	input->weap = SDL_FALSE;
	
	input->priority = 0;
	
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
	
	input->rjs_hat_id = JOY_HAT_NONE;
	input->ljs_hat_id = JOY_HAT_NONE;
	input->ujs_hat_id = JOY_HAT_NONE;
	input->djs_hat_id = JOY_HAT_NONE;
	input->weapjs_hat_id = JOY_HAT_NONE;
	
	input->joy_right_id = JOY_INVALID_ID;
	input->joy_left_id = JOY_INVALID_ID;
	input->joy_up_id = JOY_INVALID_ID;
	input->joy_down_id = JOY_INVALID_ID;
	input->joy_weap_id = JOY_INVALID_ID;
	
	// allocate some memory for the descriptions.
	input->joy_right = calloc(MAX_JOY_DESCRIPTION_BUFFER_LENGTH, 1);
	input->joy_left = calloc(MAX_JOY_DESCRIPTION_BUFFER_LENGTH, 1);
	input->joy_up = calloc(MAX_JOY_DESCRIPTION_BUFFER_LENGTH, 1);
	input->joy_down = calloc(MAX_JOY_DESCRIPTION_BUFFER_LENGTH, 1);
	input->joy_weap = calloc(MAX_JOY_DESCRIPTION_BUFFER_LENGTH, 1);
	
	// alocate memory for the uuids
	input->joy_right_guid = calloc(MAX_JOY_GUID_BUFFER_LENGTH, 1);
	input->joy_left_guid = calloc(MAX_JOY_GUID_BUFFER_LENGTH, 1);
	input->joy_up_guid = calloc(MAX_JOY_GUID_BUFFER_LENGTH, 1);
	input->joy_down_guid = calloc(MAX_JOY_GUID_BUFFER_LENGTH, 1);
	input->joy_weap_guid = calloc(MAX_JOY_GUID_BUFFER_LENGTH, 1);
	
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

static void prepareFiringFromInput(Input *input)
{
	prepareFiringCharacterWeapon(input->character, input->character->x, input->character->y, input->character->pointing_direction, 0.0f);
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
			if (input->ujs_id > VALID_ANALOG_MAGNITUDE)
			{
				if (event->jaxis.value > VALID_ANALOG_MAGNITUDE)
				{
					input->up_ticks = event->key.timestamp;
					input->priority |= 1 << 0;
				}
				else
				{
					input->up_ticks = 0;
					input->priority &= ~(1 << 0);
				}
			}
			else if (input->ujs_id < -VALID_ANALOG_MAGNITUDE)
			{
				if (event->jaxis.value < -VALID_ANALOG_MAGNITUDE)
				{
					input->up_ticks = event->key.timestamp;
					input->priority |= 1 << 0;
				}
				else
				{
					input->up_ticks = 0;
					input->priority &= ~(1 << 0);
				}
			}
			else
			{
				input->up_ticks = 0;
				input->priority &= ~(1 << 0);
			}
		}
		
		if (input->joy_down_id == event->jaxis.which && input->djs_axis_id == event->jaxis.axis)
		{
			if (input->djs_id > VALID_ANALOG_MAGNITUDE)
			{
				if (event->jaxis.value > VALID_ANALOG_MAGNITUDE)
				{
					input->down_ticks = event->key.timestamp;
					input->priority |= 1 << 1;
				}
				else
				{
					input->down_ticks = 0;
					input->priority &= ~(1 << 1);
				}
			}
			else if (input->djs_id < -VALID_ANALOG_MAGNITUDE)
			{
				if (event->jaxis.value < -VALID_ANALOG_MAGNITUDE)
				{
					input->down_ticks = event->key.timestamp;
					input->priority |= 1 << 1;
				}
				else
				{
					input->down_ticks = 0;
					input->priority &= ~(1 << 1);
				}
			}
			else
			{
				input->down_ticks = 0;
				input->priority &= ~(1 << 1);
			}
		}
		
		if (input->joy_right_id == event->jaxis.which && input->rjs_axis_id == event->jaxis.axis)
		{
			if (input->rjs_id > VALID_ANALOG_MAGNITUDE)
			{
				if (event->jaxis.value > VALID_ANALOG_MAGNITUDE)
				{
					input->right_ticks = event->key.timestamp;
					input->priority |= 1 << 2;
				}
				else
				{
					input->right_ticks = 0;
					input->priority &= ~(1 << 2);
				}
			}
			else if (input->rjs_id < -VALID_ANALOG_MAGNITUDE)
			{
				if (event->jaxis.value < -VALID_ANALOG_MAGNITUDE)
				{
					input->right_ticks = event->key.timestamp;
					input->priority |= 1 << 2;
				}
				else
				{
					input->right_ticks = 0;
					input->priority &= ~(1 << 2);
				}
			}
			else
			{
				input->right_ticks = 0;
				input->priority &= ~(1 << 2);
			}
		}
		
		if (input->joy_left_id == event->jaxis.which && input->ljs_axis_id == event->jaxis.axis)
		{
			if (input->ljs_id > VALID_ANALOG_MAGNITUDE)
			{
				if (event->jaxis.value > VALID_ANALOG_MAGNITUDE)
				{
					input->left_ticks = event->key.timestamp;
					input->priority |= 1 << 3;
				}
				else
				{
					input->left_ticks = 0;
					input->priority &= ~(1 << 3);
				}
			}
			else if (input->ljs_id < -VALID_ANALOG_MAGNITUDE)
			{
				if (event->jaxis.value < -VALID_ANALOG_MAGNITUDE)
				{
					input->left_ticks = event->key.timestamp;
					input->priority |= 1 << 3;
				}
				else
				{
					input->left_ticks = 0;
					input->priority &= ~(1 << 3);
				}
			}
			else
			{
				input->left_ticks = 0;
				input->priority &= ~(1 << 3);
			}
		}
		
		if (!input->character->weap->animationState && event->jaxis.axis == input->weapjs_axis_id && event->jaxis.which == input->joy_weap_id)
		{
			
			if (input->weapjs_id > VALID_ANALOG_MAGNITUDE)
			{
				if (event->jaxis.value > VALID_ANALOG_MAGNITUDE)
				{
					if (gGameHasStarted)
					{
						input->weap = SDL_TRUE;
					}
				}
			}
			else if (input->weapjs_id < -VALID_ANALOG_MAGNITUDE)
			{
				if (event->jaxis.value < -VALID_ANALOG_MAGNITUDE)
				{
					if (gGameHasStarted)
					{
						input->weap = SDL_TRUE;
					}
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
		
		else if (event->jbutton.which == input->joy_up_id && event->jbutton.button == input->ujs_id && input->ujs_axis_id == JOY_AXIS_NONE)
		{
			input->up_ticks = event->key.timestamp;
		}
		
		else if (event->jbutton.which == input->joy_down_id && event->jbutton.button == input->djs_id && input->djs_axis_id == JOY_AXIS_NONE)
		{
			input->down_ticks = event->key.timestamp;
		}
		
		else if (event->jbutton.which == input->joy_left_id && event->jbutton.button == input->ljs_id && input->ljs_axis_id == JOY_AXIS_NONE)
		{
			input->left_ticks = event->key.timestamp;
		}
		
		else if (event->jbutton.which == input->joy_right_id && event->jbutton.button == input->rjs_id && input->rjs_axis_id == JOY_AXIS_NONE)
		{
			input->right_ticks = event->key.timestamp;
		}
	}
	else if (event->type == SDL_JOYHATMOTION)
	{
		if (event->jhat.value != 0)
		{
			if (event->jhat.which == input->joy_weap_id && event->jhat.hat == input->weapjs_hat_id && event->jhat.value == input->weapjs_id && input->weapjs_axis_id == JOY_AXIS_NONE && !input->character->weap->animationState && !input->weap)
			{
				if (gGameHasStarted)
					input->weap = SDL_TRUE;
			}
			
			else if (event->jhat.which == input->joy_up_id && event->jhat.hat == input->ujs_hat_id && event->jhat.value == input->ujs_id && input->ujs_axis_id == JOY_AXIS_NONE)
			{
				input->up_ticks = event->key.timestamp;
			}
			
			else if (event->jhat.which == input->joy_down_id && event->jhat.hat == input->djs_hat_id && event->jhat.value == input->djs_id && input->djs_axis_id == JOY_AXIS_NONE)
			{
				input->down_ticks = event->key.timestamp;
			}
			
			else if (event->jhat.which == input->joy_left_id && event->jhat.hat == input->ljs_hat_id && event->jhat.value == input->ljs_id && input->ljs_axis_id == JOY_AXIS_NONE)
			{
				input->left_ticks = event->key.timestamp;
			}
			
			else if (event->jhat.which == input->joy_right_id && event->jhat.hat == input->rjs_hat_id && event->jhat.value == input->rjs_id && input->rjs_axis_id == JOY_AXIS_NONE)
			{
				input->right_ticks = event->key.timestamp;
			}
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
				prepareFiringFromInput(input);
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
				prepareFiringFromInput(input);
			}
			input->weap = SDL_FALSE;
		}
	}
	else if (event->type == SDL_JOYHATMOTION)
	{
		if (event->jhat.value == 0)
		{
			if (input->up_ticks != 0 && event->jhat.which == input->joy_up_id && event->jhat.hat == input->ujs_hat_id && input->ujs_axis_id == JOY_AXIS_NONE)
			{
				input->up_ticks = 0;
			}
			
			if (input->down_ticks != 0 && event->jhat.which == input->joy_down_id && event->jhat.hat == input->djs_hat_id && input->djs_axis_id == JOY_AXIS_NONE)
			{
				input->down_ticks = 0;
			}
			
			if (input->right_ticks != 0 && event->jhat.which == input->joy_right_id && event->jhat.hat == input->rjs_hat_id && input->rjs_axis_id == JOY_AXIS_NONE)
			{
				input->right_ticks = 0;
			}
			
			if (input->left_ticks != 0 && event->jhat.which == input->joy_left_id && event->jhat.hat == input->ljs_hat_id && input->ljs_axis_id == JOY_AXIS_NONE)
			{
				input->left_ticks = 0;
			}
			
			if (input->weap && event->jhat.which == input->joy_weap_id && event->jhat.hat == input->weapjs_hat_id && input->weapjs_axis_id == JOY_AXIS_NONE &&
					 !input->character->weap->animationState)
			{
				if (gGameHasStarted)
				{
					prepareFiringFromInput(input);
				}
				input->weap = SDL_FALSE;
			}
		}
	}
	else if (event->type == SDL_JOYAXISMOTION)
	{
		/* Other JOYAXISMOTION UPs are dealt with in performDownAction() (because it was easier to deal with there) */
		
		if (input->weap && !input->character->weap->animationState && input->weapjs_axis_id != JOY_AXIS_NONE)
		{
			if (gGameHasStarted)
			{
				prepareFiringFromInput(input);
			}
			input->weap = SDL_FALSE;
		}
	}
}

static uint32_t maxValue(uint32_t value, uint32_t value2)
{
	return (value > value2) ? value : value2;
}

static int bestDirectionFromInputTicks(SDL_bool preferMaxValue, uint32_t right_ticks, uint32_t left_ticks, uint32_t up_ticks, uint32_t down_ticks)
{
	if (right_ticks == 0 && left_ticks == 0 && up_ticks == 0 && down_ticks == 0)
	{
		return NO_DIRECTION;
	}
	
	uint32_t scores[] = {right_ticks, left_ticks, up_ticks, down_ticks};
	
	uint32_t bestScore = preferMaxValue ? 0 : UINT32_MAX;
	uint8_t bestScoreIndex = 0;
	
	for (uint8_t scoreIndex = 0; scoreIndex < sizeof(scores) / sizeof(scores[0]); scoreIndex++)
	{
		if ((preferMaxValue && scores[scoreIndex] > bestScore) || (!preferMaxValue && scores[scoreIndex] != 0 && scores[scoreIndex] < bestScore))
		{
			bestScore = scores[scoreIndex];
			bestScoreIndex = scoreIndex;
		}
	}
	
	return bestScoreIndex + 1;
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
	
	uint8_t priority = gRedRoverInput.priority | gPinkBubbleGumInput.priority | gGreenTreeInput.priority | gBlueLightningInput.priority;
	
	int newDirection = bestDirectionFromInputTicks(priority == 0, rightTicks, leftTicks, upTicks, downTicks);
	
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
	
	input->character->direction = bestDirectionFromInputTicks(input->priority == 0, input->right_ticks, input->left_ticks, input->up_ticks, input->down_ticks);
}

SDL_bool joyButtonEventMatchesMovementFromInput(SDL_JoyButtonEvent *buttonEvent, Input *input)
{
	if (buttonEvent->which == input->joy_right_id && buttonEvent->button == input->rjs_id)
	{
		return SDL_TRUE;
	}
	
	if (buttonEvent->which == input->joy_left_id && buttonEvent->button == input->ljs_id)
	{
		return SDL_TRUE;
	}
	
	if (buttonEvent->which == input->joy_up_id && buttonEvent->button == input->ujs_id)
	{
		return SDL_TRUE;
	}
	
	if (buttonEvent->which == input->joy_down_id && buttonEvent->button == input->djs_id)
	{
		return SDL_TRUE;
	}
	
	return SDL_FALSE;
}
