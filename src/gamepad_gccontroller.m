/*
* Copyright 2019 Mayur Pawashe
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

#import "gamepad_gccontroller.h"
#import "zgtime.h"

#import <Foundation/Foundation.h>
#import <GameController/GameController.h>

static void _removeController(struct _GamepadManager *gamepadManager, GCController *controller)
{
	for (uint16_t index = 0; index < MAX_GAMEPADS; index++)
	{
		Gamepad *gamepad = &gamepadManager->gamepads[index];
		if (gamepad->controller == (__bridge void *)(controller))
		{
			if (gamepadManager->removalCallback != NULL)
			{
				gamepadManager->removalCallback(gamepad->index, gamepadManager->context);
			}
			
			CFRelease(gamepad->controller);
			memset(gamepad, 0, sizeof(*gamepad));
			break;
		}
	}
}

static void _addController(struct _GamepadManager *gamepadManager, GCController *controller)
{
	if (controller.extendedGamepad == nil && controller.microGamepad == nil)
	{
		GCPhysicalInputProfile *physicalInputProfile = controller.physicalInputProfile;
		
		GCDeviceDirectionPad *leftThumbstickDpad = physicalInputProfile.dpads[GCInputLeftThumbstick];
		GCDeviceDirectionPad *directionPad = physicalInputProfile.dpads[GCInputDirectionPad];
		
		GCDeviceButtonInput *buttonA = physicalInputProfile.buttons[GCInputButtonA];
		GCDeviceButtonInput *buttonB = physicalInputProfile.buttons[GCInputButtonB];
		GCDeviceButtonInput *optionsButton = physicalInputProfile.buttons[GCInputButtonOptions];
		
		if ((leftThumbstickDpad == nil && directionPad == nil) || (buttonA == nil) || (buttonB == nil && optionsButton == nil))
		{
			return;
		}
	}
	
	uint16_t availableGamepadIndex = MAX_GAMEPADS;
	for (uint16_t index = 0; index < MAX_GAMEPADS; index++)
	{
		Gamepad *gamepad = &gamepadManager->gamepads[index];
		if (gamepad->controller == (__bridge void *)(controller))
		{
			return;
		}
		else if (gamepad->controller == nil
#if GC_KEYBOARD
				 && gamepad->keyboard == nil
#endif
				 && availableGamepadIndex == MAX_GAMEPADS)
		{
			availableGamepadIndex = index;
		}
	}
	
	if (availableGamepadIndex < MAX_GAMEPADS)
	{
		GCMicroGamepad *microGamepad = controller.microGamepad;
		if (microGamepad != nil)
		{
			microGamepad.allowsRotation = YES;
		}
		
		Gamepad *gamepad = &gamepadManager->gamepads[availableGamepadIndex];
		gamepad->controller = (void *)CFBridgingRetain(controller);
		gamepad->rank = (microGamepad != nil && controller.extendedGamepad == nil) ? LOWEST_GAMEPAD_RANK : 4;
		
		NSString *vendorName = controller.vendorName;
		NSString *productDescription;
		NSString *productCategory = controller.productCategory;
		if (vendorName != nil)
		{
			productDescription = [NSString stringWithFormat:@"%@ %@", vendorName, productCategory];
		}
		else
		{
			productDescription = productCategory;
		}
		
		const char *productDescriptionUTF8 = [productDescription UTF8String];
		if (productDescriptionUTF8 != NULL)
		{
			strncpy(gamepad->name, productDescriptionUTF8, sizeof(gamepad->name) - 1);
		}
		
		gamepad->index = gamepadManager->nextGamepadIndex;
		gamepadManager->nextGamepadIndex++;
		
		if (gamepadManager->addedCallback != NULL)
		{
			gamepadManager->addedCallback(gamepad->index, gamepadManager->context);
		}
	}
}

#if GC_KEYBOARD
static void _removeKeyboard(struct _GamepadManager *gamepadManager, GCKeyboard *keyboard) API_AVAILABLE(ios(14.0), tvos(14.0))
{
	for (uint16_t index = 0; index < MAX_GAMEPADS; index++)
	{
		Gamepad *gamepad = &gamepadManager->gamepads[index];
		// Only one keyboard can be available
		assert(gamepad->keyboard == NULL || gamepad->keyboard == (__bridge void *)(keyboard));
		if (gamepad->keyboard != NULL)
		{
			if (gamepadManager->removalCallback != NULL)
			{
				gamepadManager->removalCallback(gamepad->index, gamepadManager->context);
			}
			
			CFRelease(gamepad->keyboard);
			memset(gamepad, 0, sizeof(*gamepad));
			break;
		}
	}
}

static void _addKeyboard(struct _GamepadManager *gamepadManager, GCKeyboard *keyboard) API_AVAILABLE(ios(14.0), tvos(14.0))
{
	GCKeyboardInput *keyboardInput = keyboard.keyboardInput;
	if (keyboardInput == nil)
	{
		return;
	}
	
	uint16_t availableGamepadIndex = MAX_GAMEPADS;
	for (uint16_t index = 0; index < MAX_GAMEPADS; index++)
	{
		Gamepad *gamepad = &gamepadManager->gamepads[index];
		// Only one keyboard can be available
		assert(gamepad->keyboard == NULL || gamepad->keyboard == (__bridge void *)(keyboard));
		if (gamepad->keyboard != NULL)
		{
			return;
		}
		else if (gamepad->controller == nil && gamepad->keyboard == nil && availableGamepadIndex == MAX_GAMEPADS)
		{
			availableGamepadIndex = index;
		}
	}
	
	if (availableGamepadIndex < MAX_GAMEPADS)
	{
		Gamepad *gamepad = &gamepadManager->gamepads[availableGamepadIndex];
		gamepad->keyboard = (void *)CFBridgingRetain(keyboard);
		gamepad->rank = 2;
		strncpy(gamepad->name, "Keyboard", sizeof(gamepad->name) - 1);
		// There will only ever be one keyboard instance, reserve it for last value
		gamepad->index = UINT32_MAX - 1;
		
		if (gamepadManager->addedCallback != NULL)
		{
			gamepadManager->addedCallback(gamepad->index, gamepadManager->context);
		}
	}
}
#endif

struct _GamepadManager *initGamepadManager(const char *databasePath, GamepadCallback addedCallback, GamepadCallback removalCallback, void *context)
{
	struct _GamepadManager *gamepadManager = calloc(1, sizeof(*gamepadManager));
	gamepadManager->addedCallback = addedCallback;
	gamepadManager->removalCallback = removalCallback;
	gamepadManager->context = context;
	gamepadManager->nextGamepadIndex = 0;
	
	// Make sure we don't dispatch too early before returning gamepad manager
	dispatch_async(dispatch_get_main_queue(), ^{
		for (GCController *controller in [GCController controllers])
		{
			_addController(gamepadManager, controller);
		}
		
#if GC_KEYBOARD
		GCKeyboard *keyboard = GCKeyboard.coalescedKeyboard;
		if (keyboard != nil)
		{
			_addKeyboard(gamepadManager, keyboard);
		}
#endif
	});
	
	[[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidConnectNotification object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
		_addController(gamepadManager, [note object]);
	}];
	
	[[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidDisconnectNotification object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
		_removeController(gamepadManager, [note object]);
	}];
	
#if GC_KEYBOARD
	[[NSNotificationCenter defaultCenter] addObserverForName:GCKeyboardDidConnectNotification object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
		_addKeyboard(gamepadManager, [note object]);
	}];
	
	[[NSNotificationCenter defaultCenter] addObserverForName:GCKeyboardDidDisconnectNotification object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
		_removeKeyboard(gamepadManager, [note object]);
	}];
#endif
	
	return gamepadManager;
}

static void _addButtonEventIfNeeded(Gamepad *gamepad, GamepadButton button, BOOL buttonPressed, GamepadEvent *eventsBuffer, uint16_t *eventIndex)
{
	if ((GamepadState)buttonPressed != gamepad->lastStates[button])
	{
		gamepad->lastStates[button] = (GamepadState)buttonPressed;
		
		GamepadState state = buttonPressed ? GAMEPAD_STATE_PRESSED : GAMEPAD_STATE_RELEASED;
		GamepadEvent event = {.button = button, .index = gamepad->index, .state = state, .mappingType = GAMEPAD_ELEMENT_MAPPING_TYPE_BUTTON, .ticks = ZGGetNanoTicks()};
		
		eventsBuffer[*eventIndex] = event;
		(*eventIndex)++;
	}
}

static void _addAxisEventIfNeeded(Gamepad *gamepad, GamepadButton positiveButton, GamepadButton negativeButton, GCControllerAxisInput *axisInput, GamepadEvent *eventsBuffer, uint16_t *eventIndex)
{
	float axisValue = axisInput.value;
	
	_addButtonEventIfNeeded(gamepad, positiveButton, axisValue >= AXIS_THRESHOLD_PERCENT, eventsBuffer, eventIndex);
	_addButtonEventIfNeeded(gamepad, negativeButton, axisValue <= -AXIS_THRESHOLD_PERCENT, eventsBuffer, eventIndex);
}

GamepadEvent *pollGamepadEvents(struct _GamepadManager *gamepadManager, const void *systemEvent, uint16_t *eventCount)
{
	if (systemEvent != NULL)
	{
		*eventCount = 0;
		return NULL;
	}
	
	uint16_t eventIndex = 0;
	for (uint16_t index = 0; index < MAX_GAMEPADS; index++)
	{
		Gamepad *gamepad = &gamepadManager->gamepads[index];
		GCController *controller = (__bridge GCController *)(gamepad->controller);
#if GC_KEYBOARD
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability-new"
		GCKeyboard *keyboard = (__bridge GCKeyboard *)(gamepad->keyboard);
#pragma clang diagnostic pop
#endif
		
		if (controller != nil)
		{
			GCExtendedGamepad *extendedGamepad = controller.extendedGamepad;
			GCMicroGamepad *microGamepad;
			if (extendedGamepad != nil)
			{
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_A, extendedGamepad.buttonA.pressed, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_B, extendedGamepad.buttonB.pressed, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_X, extendedGamepad.buttonX.pressed, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_Y, extendedGamepad.buttonY.pressed, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_LEFTSHOULDER, extendedGamepad.leftShoulder.pressed, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_RIGHTSHOULDER, extendedGamepad.rightShoulder.pressed, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_LEFTTRIGGER, extendedGamepad.leftTrigger.pressed, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_RIGHTTRIGGER, extendedGamepad.rightTrigger.pressed, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_START, extendedGamepad.buttonMenu.pressed, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_BACK, extendedGamepad.buttonOptions.pressed, gamepadManager->eventsBuffer, &eventIndex);
				
				BOOL upPressed = extendedGamepad.dpad.up.pressed;
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_UP, upPressed, gamepadManager->eventsBuffer, &eventIndex);
				
				BOOL downPressed = extendedGamepad.dpad.down.pressed;
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_DOWN, downPressed, gamepadManager->eventsBuffer, &eventIndex);
				
				BOOL leftPressed = extendedGamepad.dpad.left.pressed;
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_LEFT, leftPressed, gamepadManager->eventsBuffer, &eventIndex);
				
				BOOL rightPressed = extendedGamepad.dpad.right.pressed;
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_RIGHT, rightPressed, gamepadManager->eventsBuffer, &eventIndex);
				
				if (!upPressed && !downPressed && !leftPressed && !rightPressed)
				{
					_addAxisEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_RIGHT, GAMEPAD_BUTTON_DPAD_LEFT, extendedGamepad.leftThumbstick.xAxis, gamepadManager->eventsBuffer, &eventIndex);
					_addAxisEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_UP, GAMEPAD_BUTTON_DPAD_DOWN, extendedGamepad.leftThumbstick.yAxis, gamepadManager->eventsBuffer, &eventIndex);
				}
			}
			else if ((microGamepad = controller.microGamepad) != nil)
			{
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_A, microGamepad.buttonA.pressed, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_B, microGamepad.buttonX.pressed, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_START, microGamepad.buttonMenu.pressed, gamepadManager->eventsBuffer, &eventIndex);
				
				_addAxisEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_RIGHT, GAMEPAD_BUTTON_DPAD_LEFT, microGamepad.dpad.xAxis, gamepadManager->eventsBuffer, &eventIndex);
				_addAxisEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_UP, GAMEPAD_BUTTON_DPAD_DOWN, microGamepad.dpad.yAxis, gamepadManager->eventsBuffer, &eventIndex);
			}
			else
			{
				// This path will support more non-traditional controllers
				// We will prefer extended and micro gamepads (which are more reliable) before using a physical input profile
				
				GCPhysicalInputProfile *physicalInputProfile = controller.physicalInputProfile;
				
				GCDeviceDirectionPad *dpad = physicalInputProfile.dpads[GCInputDirectionPad];
				GCDeviceDirectionPad *leftThumbstick = physicalInputProfile.dpads[GCInputLeftThumbstick];
				
				GCDeviceButtonInput *buttonA = physicalInputProfile.buttons[GCInputButtonA];
				GCDeviceButtonInput *buttonB = physicalInputProfile.buttons[GCInputButtonB];
				GCDeviceButtonInput *buttonX = physicalInputProfile.buttons[GCInputButtonX];
				GCDeviceButtonInput *buttonY = physicalInputProfile.buttons[GCInputButtonY];
				
				GCDeviceButtonInput *leftShoulder = physicalInputProfile.buttons[GCInputLeftShoulder];
				GCDeviceButtonInput *rightShoulder = physicalInputProfile.buttons[GCInputRightShoulder];
				
				GCDeviceButtonInput *leftTrigger = physicalInputProfile.buttons[GCInputLeftTrigger];
				GCDeviceButtonInput *rightTrigger = physicalInputProfile.buttons[GCInputRightTrigger];
				
				GCDeviceButtonInput *buttonMenu = physicalInputProfile.buttons[GCInputButtonMenu];
				GCDeviceButtonInput *buttonOptions = physicalInputProfile.buttons[GCInputButtonOptions];
				
				if (buttonA != nil)
				{
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_A, buttonA.pressed, gamepadManager->eventsBuffer, &eventIndex);
				}
				
				if (buttonB != nil)
				{
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_B, buttonB.pressed, gamepadManager->eventsBuffer, &eventIndex);
				}
				
				if (buttonX != nil)
				{
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_X, buttonX.pressed, gamepadManager->eventsBuffer, &eventIndex);
				}
				
				if (buttonY != nil)
				{
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_Y, buttonY.pressed, gamepadManager->eventsBuffer, &eventIndex);
				}
				
				if (leftShoulder != nil)
				{
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_LEFTSHOULDER, leftShoulder.pressed, gamepadManager->eventsBuffer, &eventIndex);
				}
				
				if (rightShoulder != nil)
				{
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_RIGHTSHOULDER, rightShoulder.pressed, gamepadManager->eventsBuffer, &eventIndex);
				}
				
				if (leftTrigger != nil)
				{
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_LEFTTRIGGER, leftTrigger.pressed, gamepadManager->eventsBuffer, &eventIndex);
				}
				
				if (rightTrigger != nil)
				{
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_RIGHTTRIGGER, rightTrigger.pressed, gamepadManager->eventsBuffer, &eventIndex);
				}
				
				if (buttonMenu != nil)
				{
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_START, buttonMenu.pressed, gamepadManager->eventsBuffer, &eventIndex);
				}
				
				if (buttonOptions != nil)
				{
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_BACK, buttonOptions.pressed, gamepadManager->eventsBuffer, &eventIndex);
				}
				
				// It's more reliable to only check for presence of dpad or left thumbstick and to use the axis values for the dpad
				if (dpad != nil)
				{
					_addAxisEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_RIGHT, GAMEPAD_BUTTON_DPAD_LEFT, dpad.xAxis, gamepadManager->eventsBuffer, &eventIndex);
					_addAxisEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_UP, GAMEPAD_BUTTON_DPAD_DOWN, dpad.yAxis, gamepadManager->eventsBuffer, &eventIndex);
				}
				else if (leftThumbstick != nil)
				{
					_addAxisEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_RIGHT, GAMEPAD_BUTTON_DPAD_LEFT, leftThumbstick.xAxis, gamepadManager->eventsBuffer, &eventIndex);
					_addAxisEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_UP, GAMEPAD_BUTTON_DPAD_DOWN, leftThumbstick.yAxis, gamepadManager->eventsBuffer, &eventIndex);
				}
			}
		}
#if GC_KEYBOARD
		else if (keyboard != nil)
		{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability-new"
			GCKeyboardInput *keyboardInput = keyboard.keyboardInput;
			if (keyboardInput != nil)
			{
				GCControllerButtonInput *upInput = [keyboardInput buttonForKeyCode:GCKeyCodeUpArrow];
				GCControllerButtonInput *upInput2 = [keyboardInput buttonForKeyCode:GCKeyCodeKeyW];
				
				if (upInput != nil || upInput2 != nil)
				{
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_UP, upInput.pressed || upInput2.pressed, gamepadManager->eventsBuffer, &eventIndex);
				}
				
				GCControllerButtonInput *downInput = [keyboardInput buttonForKeyCode:GCKeyCodeDownArrow];
				GCControllerButtonInput *downInput2 = [keyboardInput buttonForKeyCode:GCKeyCodeKeyS];
				
				if (downInput != nil || downInput2 != nil)
				{
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_DOWN, downInput.pressed || downInput2.pressed, gamepadManager->eventsBuffer, &eventIndex);
				}
				
				GCControllerButtonInput *rightInput = [keyboardInput buttonForKeyCode:GCKeyCodeRightArrow];
				GCControllerButtonInput *rightInput2 = [keyboardInput buttonForKeyCode:GCKeyCodeKeyD];
				
				if (rightInput != nil || rightInput2 != nil)
				{
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_RIGHT, rightInput.pressed || rightInput2.pressed, gamepadManager->eventsBuffer, &eventIndex);
				}
				
				GCControllerButtonInput *leftInput = [keyboardInput buttonForKeyCode:GCKeyCodeLeftArrow];
				GCControllerButtonInput *leftInput2 = [keyboardInput buttonForKeyCode:GCKeyCodeKeyA];
				
				if (leftInput != nil || leftInput2 != nil)
				{
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_LEFT, leftInput.pressed || leftInput2.pressed, gamepadManager->eventsBuffer, &eventIndex);
				}
				
				GCControllerButtonInput *actionInput = [keyboardInput buttonForKeyCode:GCKeyCodeReturnOrEnter];
				if (actionInput != nil)
				{
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_A, actionInput.pressed, gamepadManager->eventsBuffer, &eventIndex);
				}
				
				GCControllerButtonInput *action2Input = [keyboardInput buttonForKeyCode:GCKeyCodeSpacebar];
				if (action2Input != nil)
				{
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_X, action2Input.pressed, gamepadManager->eventsBuffer, &eventIndex);
				}
				
				GCControllerButtonInput *startInput = [keyboardInput buttonForKeyCode:GCKeyCodeEscape];
				if (startInput != nil)
				{
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_START, startInput.pressed, gamepadManager->eventsBuffer, &eventIndex);
				}
			}
#pragma clang diagnostic pop
		}
#endif
	}
	
	*eventCount = eventIndex;
	return gamepadManager->eventsBuffer;
}

const char *gamepadName(struct _GamepadManager *gamepadManager, GamepadIndex gcIndex)
{
	for (uint16_t index = 0; index < MAX_GAMEPADS; index++)
	{
		Gamepad *gamepad = &gamepadManager->gamepads[index];
		if ((
			 gamepad->controller != NULL
#if GC_KEYBOARD
			 || gamepad->keyboard != NULL
#endif
			 )
			&& gamepad->index == gcIndex)
		{
			return gamepad->name;
		}
	}
	return NULL;
}

uint8_t gamepadRank(struct _GamepadManager *gamepadManager, GamepadIndex gcIndex)
{
	for (uint16_t index = 0; index < MAX_GAMEPADS; index++)
	{
		Gamepad *gamepad = &gamepadManager->gamepads[index];
		if ((
			 gamepad->controller != NULL
#if GC_KEYBOARD
			 || gamepad->keyboard != NULL
#endif
			 )
			&& gamepad->index == gcIndex)
		{
			return gamepad->rank;
		}
	}
	
	return 0;
}

void setPlayerIndex(struct _GamepadManager *gamepadManager, GamepadIndex gcIndex, int64_t playerIndex)
{
	for (uint16_t index = 0; index < MAX_GAMEPADS; index++)
	{
		Gamepad *gamepad = &gamepadManager->gamepads[index];
		
		GCController *controller = (__bridge GCController *)(gamepad->controller);
		if (controller != nil && gamepad->index == gcIndex)
		{
			controller.playerIndex = playerIndex;
			break;
		}
	}
}
