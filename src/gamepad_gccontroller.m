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

#if GC_PRODUCT_CHECK

#import <IOKit/hid/IOHIDKeys.h>

typedef void* IOHIDServiceClientRef;
extern CFTypeRef IOHIDServiceClientCopyProperty(IOHIDServiceClientRef service, CFStringRef key);

@interface GCController (Private)

- (NSArray *)hidServices;

@end

@interface NSObject (Private)

- (IOHIDServiceClientRef)service;

@end

bool GC_NAME(availableGamepadProfile)(int32_t vendorID, int32_t productID)
{
	BOOL gcXboxOneDualShockSupport;
	if (@available(macOS 10.15, iOS 13, *))
	{
		gcXboxOneDualShockSupport = YES;
	}
	else
	{
		gcXboxOneDualShockSupport = NO;
	}
	
	// These are some of the controllers we know GCController supports
	return ((gcXboxOneDualShockSupport && vendorID == 0x45e && (productID == 0x2e0 || productID == 0x2fd)) || // Xbox One
		(gcXboxOneDualShockSupport && vendorID == 0x54c && (productID == 0x9cc || productID == 0x5c4)) || // Dual Shock 4
		(vendorID == 0x1038 && productID == 0x1420) || // SteelSeries Nimbus
		(vendorID == 0x0111 && productID == 0x1420) || // SteelSeries Nimbus (wireless)
		(vendorID == 0x1038 && productID == 0x5) || // SteelSeries Stratus (possibly)
		(vendorID == 0x0F0D && productID == 0x0090) || // HoriPad Ultimate
		(vendorID == 0x0738 && productID == 0x5262)); // Mad Catz Micro C.T.R.L.i, although I'm not too sure
}

// https://stackoverflow.com/questions/33509296/supporting-both-gccontroller-and-iohiddeviceref
static bool _isSupportedController(GCController *controller)
{
	if ([controller respondsToSelector:@selector(hidServices)])
	{
		NSArray *hidServices = [controller hidServices];
		for (id hidServiceInfo in hidServices)
		{
			if ([hidServiceInfo respondsToSelector:@selector(service)])
			{
				IOHIDServiceClientRef service = [hidServiceInfo service];
				if (service != NULL)
				{
					NSNumber *vendorRef = CFBridgingRelease(IOHIDServiceClientCopyProperty(service, CFSTR(kIOHIDVendorIDKey)));
					NSNumber *productRef = CFBridgingRelease(IOHIDServiceClientCopyProperty(service, CFSTR(kIOHIDProductIDKey)));
					
					if (GC_NAME(availableGamepadProfile)(vendorRef.intValue, productRef.intValue))
					{
						return true;
					}
				}
			}
		}
	}
	return false;
}

#endif

static void _removeController(struct GC_NAME(_GamepadManager) *gamepadManager, GCController *controller)
{
	for (uint16_t index = 0; index < MAX_GAMEPADS; index++)
	{
		GC_NAME(Gamepad) *gamepad = &gamepadManager->gamepads[index];
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

static void _addController(struct GC_NAME(_GamepadManager) *gamepadManager, GCController *controller)
{
	if (controller.extendedGamepad == nil && controller.microGamepad == nil)
	{
		return;
	}
	
#if GC_PRODUCT_CHECK
	if (!_isSupportedController(controller))
	{
		return;
	}
#endif
	
	uint16_t availableGamepadIndex = MAX_GAMEPADS;
	for (uint16_t index = 0; index < MAX_GAMEPADS; index++)
	{
		GC_NAME(Gamepad) *gamepad = &gamepadManager->gamepads[index];
		if (gamepad->controller == (__bridge void *)(controller))
		{
			return;
		}
		else if (gamepad->controller == nil && availableGamepadIndex == MAX_GAMEPADS)
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
		
		GC_NAME(Gamepad) *gamepad = &gamepadManager->gamepads[availableGamepadIndex];
		gamepad->controller = (void *)CFBridgingRetain(controller);
		NSString *vendorName = controller.vendorName;
		NSString *productDescription;
		if (@available(macOS 10.15, iOS 13, *))
		{
			NSString *productCategory = controller.productCategory;
			if (vendorName != nil)
			{
				productDescription = [NSString stringWithFormat:@"%@ %@", vendorName, productCategory];
			}
			else
			{
				productDescription = productCategory;
			}
		}
		else if (vendorName != nil)
		{
			productDescription = vendorName;
		}
		else
		{
			productDescription = @"Gamepad";
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

struct GC_NAME(_GamepadManager) *GC_NAME(initGamepadManager)(const char *databasePath, GamepadCallback addedCallback, GamepadCallback removalCallback, void *context)
{
	struct GC_NAME(_GamepadManager) *gamepadManager = calloc(1, sizeof(*gamepadManager));
	gamepadManager->addedCallback = addedCallback;
	gamepadManager->removalCallback = removalCallback;
	gamepadManager->context = context;
	
	// On macOS, there is a HID gamepad implementation as well.
	// Offset our gamepad indexes large enough so that they won't collide realistically
	gamepadManager->nextGamepadIndex = UINT32_MAX / 2;
	
	for (GCController *controller in [GCController controllers])
	{
		_addController(gamepadManager, controller);
	}
	
	[[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidConnectNotification object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
		_addController(gamepadManager, [note object]);
	}];
	
	[[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidDisconnectNotification object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
		_removeController(gamepadManager, [note object]);
	}];
	
	return gamepadManager;
}

static void _addButtonEventIfNeeded(GC_NAME(Gamepad) *gamepad, GamepadButton button, BOOL buttonPressed, GamepadEvent *eventsBuffer, uint16_t *eventIndex)
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

static void _addAxisEventIfNeeded(GC_NAME(Gamepad) *gamepad, GamepadButton positiveButton, GamepadButton negativeButton, GCControllerAxisInput *axisInput, GamepadEvent *eventsBuffer, uint16_t *eventIndex)
{
	float axisValue = axisInput.value;
	
	_addButtonEventIfNeeded(gamepad, positiveButton, axisValue >= 0.6f, eventsBuffer, eventIndex);
	_addButtonEventIfNeeded(gamepad, negativeButton, axisValue <= -0.6f, eventsBuffer, eventIndex);
}

GamepadEvent *GC_NAME(pollGamepadEvents)(struct GC_NAME(_GamepadManager) *gamepadManager, const void *systemEvent, uint16_t *eventCount)
{
	if (systemEvent != NULL)
	{
		*eventCount = 0;
		return NULL;
	}
	
	uint16_t eventIndex = 0;
	for (uint16_t index = 0; index < MAX_GAMEPADS; index++)
	{
		GC_NAME(Gamepad) *gamepad = &gamepadManager->gamepads[index];
		GCController *controller = (__bridge GCController *)(gamepad->controller);
		if (controller != nil)
		{
			GCExtendedGamepad *extendedGamepad = controller.extendedGamepad;
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
				if (@available(iOS 13.0, macOS 10.15, *))
				{
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_START, extendedGamepad.buttonMenu.pressed, gamepadManager->eventsBuffer, &eventIndex);
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_BACK, extendedGamepad.buttonOptions.pressed, gamepadManager->eventsBuffer, &eventIndex);
				}
				
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
			else
			{
				GCMicroGamepad *microGamepad = controller.microGamepad;
				if (microGamepad != nil)
				{
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_A, microGamepad.buttonA.pressed, gamepadManager->eventsBuffer, &eventIndex);
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_B, microGamepad.buttonX.pressed, gamepadManager->eventsBuffer, &eventIndex);
					if (@available(iOS 13.0, macOS 10.15, *))
					{
						_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_START, microGamepad.buttonMenu.pressed, gamepadManager->eventsBuffer, &eventIndex);
					}
					
					_addAxisEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_RIGHT, GAMEPAD_BUTTON_DPAD_LEFT, microGamepad.dpad.xAxis, gamepadManager->eventsBuffer, &eventIndex);
					_addAxisEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_UP, GAMEPAD_BUTTON_DPAD_DOWN, microGamepad.dpad.yAxis, gamepadManager->eventsBuffer, &eventIndex);
				}
			}
		}
	}
	
	*eventCount = eventIndex;
	return gamepadManager->eventsBuffer;
}

const char *GC_NAME(gamepadName)(struct GC_NAME(_GamepadManager) *gamepadManager, GamepadIndex gcIndex)
{
	for (uint16_t index = 0; index < MAX_GAMEPADS; index++)
	{
		GC_NAME(Gamepad) *gamepad = &gamepadManager->gamepads[index];
		if (gamepad->controller != NULL && gamepad->index == gcIndex)
		{
			return gamepad->name;
		}
	}
	return NULL;
}

void GC_NAME(setPlayerIndex)(struct GC_NAME(_GamepadManager) *gamepadManager, GamepadIndex gcIndex, int64_t playerIndex)
{
	for (uint16_t index = 0; index < MAX_GAMEPADS; index++)
	{
		GC_NAME(Gamepad) *gamepad = &gamepadManager->gamepads[index];
		
		GCController *controller = (__bridge GCController *)(gamepad->controller);
		if (controller != nil && gamepad->index == gcIndex)
		{
			controller.playerIndex = playerIndex;
			break;
		}
	}
}
