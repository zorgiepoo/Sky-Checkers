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
#include "globals.h"

Input gRedRoverInput;
Input gGreenTreeInput;
Input gPinkBubbleGumInput;
Input gBlueLightningInput;

void initInput(Input *input, uint32_t right, uint32_t left, uint32_t up, uint32_t down, uint32_t weapon)
{
	input->right_ticks = 0;
	input->left_ticks = 0;
	input->up_ticks = 0;
	input->down_ticks = 0;
	input->weap = false;
	
	input->priority = 0;
	
	input->r_id = right;
	input->l_id = left;
	input->u_id = up;
	input->d_id = down;
	input->weap_id = weapon;
	
	input->gamepadIndex = INVALID_GAMEPAD_INDEX;
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

void performDownKeyAction(Input *input, SDL_Event *event)
{
	if (event->key.keysym.scancode == input->weap_id && !input->character->weap->animationState)
	{
		if (gGameHasStarted)
		{
			input->weap = true;
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

void performUpKeyAction(Input *input, SDL_Event *event)
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
		input->weap = false;
	}
}

void performGamepadAction(Input *input, GamepadEvent *gamepadEvent)
{
	if (input->gamepadIndex != gamepadEvent->index) return;
	
	switch (gamepadEvent->state)
	{
		case GAMEPAD_STATE_PRESSED:
		{
			switch (gamepadEvent->button)
			{
				case GAMEPAD_BUTTON_A:
				case GAMEPAD_BUTTON_B:
				case GAMEPAD_BUTTON_X:
				case GAMEPAD_BUTTON_Y:
					if (!input->character->weap->animationState && !input->weap)
					{
						if (gGameHasStarted)
						{
							input->weap = true;
						}
					}
					break;
				case GAMEPAD_BUTTON_BACK:
				case GAMEPAD_BUTTON_START:
					break;
				case GAMEPAD_BUTTON_LEFTSHOULDER:
				case GAMEPAD_BUTTON_RIGHTSHOULDER:
				case GAMEPAD_BUTTON_LEFTTRIGGER:
				case GAMEPAD_BUTTON_RIGHTTRIGGER:
					break;
				case GAMEPAD_BUTTON_DPAD_UP:
					if (gamepadEvent->mappingType == GAMEPAD_ELEMENT_MAPPING_TYPE_AXIS)
					{
						input->priority |= 1 << 0;
					}
					input->up_ticks = gamepadEvent->ticks;
					break;
				case GAMEPAD_BUTTON_DPAD_DOWN:
					if (gamepadEvent->mappingType == GAMEPAD_ELEMENT_MAPPING_TYPE_AXIS)
					{
						input->priority |= 1 << 1;
					}
					input->down_ticks = gamepadEvent->ticks;
					break;
				case GAMEPAD_BUTTON_DPAD_RIGHT:
					if (gamepadEvent->mappingType == GAMEPAD_ELEMENT_MAPPING_TYPE_AXIS)
					{
						input->priority |= 1 << 2;
					}
					input->right_ticks = gamepadEvent->ticks;
					break;
				case GAMEPAD_BUTTON_DPAD_LEFT:
					if (gamepadEvent->mappingType == GAMEPAD_ELEMENT_MAPPING_TYPE_AXIS)
					{
						input->priority |= 1 << 3;
					}
					input->left_ticks = gamepadEvent->ticks;
					break;
				case GAMEPAD_BUTTON_MAX:
					break;
			}
			break;
		}
		case GAMEPAD_STATE_RELEASED:
		{
			switch (gamepadEvent->button)
			{
				case GAMEPAD_BUTTON_A:
				case GAMEPAD_BUTTON_B:
				case GAMEPAD_BUTTON_X:
				case GAMEPAD_BUTTON_Y:
					if (input->weap && !input->character->weap->animationState)
					{
						if (gGameHasStarted)
						{
							prepareFiringFromInput(input);
						}
						input->weap = false;
					}
					break;
				case GAMEPAD_BUTTON_BACK:
				case GAMEPAD_BUTTON_START:
					break;
				case GAMEPAD_BUTTON_LEFTSHOULDER:
				case GAMEPAD_BUTTON_RIGHTSHOULDER:
				case GAMEPAD_BUTTON_LEFTTRIGGER:
				case GAMEPAD_BUTTON_RIGHTTRIGGER:
					break;
				case GAMEPAD_BUTTON_DPAD_UP:
					if (gamepadEvent->mappingType == GAMEPAD_ELEMENT_MAPPING_TYPE_AXIS)
					{
						input->priority &= ~(1 << 0);
					}
					input->up_ticks = 0;
					break;
				case GAMEPAD_BUTTON_DPAD_DOWN:
					if (gamepadEvent->mappingType == GAMEPAD_ELEMENT_MAPPING_TYPE_AXIS)
					{
						input->priority &= ~(1 << 1);
					}
					input->down_ticks = 0;
					break;
				case GAMEPAD_BUTTON_DPAD_RIGHT:
					if (gamepadEvent->mappingType == GAMEPAD_ELEMENT_MAPPING_TYPE_AXIS)
					{
						input->priority &= ~(1 << 2);
					}
					input->right_ticks = 0;
					break;
				case GAMEPAD_BUTTON_DPAD_LEFT:
					if (gamepadEvent->mappingType == GAMEPAD_ELEMENT_MAPPING_TYPE_AXIS)
					{
						input->priority &= ~(1 << 3);
					}
					input->left_ticks = 0;
					break;
				case GAMEPAD_BUTTON_MAX:
					break;
			}
			break;
		}
	}
}

static uint32_t maxValue(uint32_t value, uint32_t value2)
{
	return (value > value2) ? value : value2;
}

static int bestDirectionFromInputTicks(bool preferMaxValue, uint32_t right_ticks, uint32_t left_ticks, uint32_t up_ticks, uint32_t down_ticks)
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
