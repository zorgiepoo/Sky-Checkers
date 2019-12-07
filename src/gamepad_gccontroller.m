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
#import "time.h"

#import <Foundation/Foundation.h>
#import <GameController/GameController.h>

#if USE_GC_SPI

#import <IOKit/hid/IOHIDKeys.h>

typedef void* IOHIDServiceClientRef;
extern CFTypeRef IOHIDServiceClientCopyProperty(IOHIDServiceClientRef service, CFStringRef key);

@interface GCController (Private)

- (NSArray *)hidServices;

@end

@interface NSObject (Private)

- (IOHIDServiceClientRef)service;

@end

// https://stackoverflow.com/questions/33509296/supporting-both-gccontroller-and-iohiddeviceref
static void _retrieveVendorAndProductIDs(GC_NAME(Gamepad) *gamepad, GCController *controller)
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
					NSNumber *vendorID = CFBridgingRelease(IOHIDServiceClientCopyProperty(service, CFSTR(kIOHIDVendorIDKey)));
					if (vendorID != nil)
					{
						gamepad->vendorID = vendorID.intValue;
					}
					
					NSNumber *productID = CFBridgingRelease(IOHIDServiceClientCopyProperty(service, CFSTR(kIOHIDProductIDKey)));
					if (productID != nil)
					{
						gamepad->productID = productID.intValue;
					}
				}
			}
			break;
		}
	}
}

#endif

static void _removeController(struct GC_NAME(_GamepadManager) *gamepadManager, GCController *controller)
{
	for (uint16_t gamepadIndex = 0; gamepadIndex < MAX_GAMEPADS; gamepadIndex++)
	{
		GC_NAME(Gamepad) *gamepad = &gamepadManager->gamepads[gamepadIndex];
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
	uint16_t availableGamepadIndex = MAX_GAMEPADS;
	for (uint16_t gamepadIndex = 0; gamepadIndex < MAX_GAMEPADS; gamepadIndex++)
	{
		GC_NAME(Gamepad) *gamepad = &gamepadManager->gamepads[gamepadIndex];
		if (gamepad->controller == (__bridge void *)(controller))
		{
			return;
		}
		else if (gamepad->controller == nil && availableGamepadIndex == MAX_GAMEPADS)
		{
			availableGamepadIndex = gamepadIndex;
		}
	}
	
	if (availableGamepadIndex < MAX_GAMEPADS)
	{
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
		else
		{
			productDescription = vendorName;
		}
		
		const char *productDescriptionUTF8 = [productDescription UTF8String];
		if (productDescriptionUTF8 != NULL)
		{
			strncpy(gamepad->name, productDescriptionUTF8, sizeof(gamepad->name) - 1);
		}
		
		gamepad->index = gamepadManager->nextGamepadIndex;
#if USE_GC_SPI
		_retrieveVendorAndProductIDs(gamepad, controller);
#endif
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

static void _addButtonEventIfNeeded(GC_NAME(Gamepad) *gamepad, GamepadButton button, GCControllerButtonInput *buttonInput, GamepadEvent *eventsBuffer, uint16_t *eventIndex)
{
	BOOL pressed = buttonInput.pressed;
	if ((GamepadState)pressed != gamepad->lastStates[button])
	{
		gamepad->lastStates[button] = pressed;
		
		GamepadState state = pressed ? GAMEPAD_STATE_PRESSED : GAMEPAD_STATE_RELEASED;
		
		GamepadEvent event = {.button = button, .index = gamepad->index, .state = state, .mappingType = GAMEPAD_ELEMENT_MAPPING_TYPE_BUTTON, .ticks = ZGGetTicks()};
		
		eventsBuffer[*eventIndex] = event;
		(*eventIndex)++;
	}
}

GamepadEvent *GC_NAME(pollGamepadEvents)(struct GC_NAME(_GamepadManager) *gamepadManager, const void *systemEvent, uint16_t *eventCount)
{
	if (systemEvent != NULL)
	{
		*eventCount = 0;
		return NULL;
	}
	
	uint16_t eventIndex = 0;
	for (uint16_t gamepadIndex = 0; gamepadIndex < MAX_GAMEPADS; gamepadIndex++)
	{
		GC_NAME(Gamepad) *gamepad = &gamepadManager->gamepads[gamepadIndex];
		GCController *controller = (__bridge GCController *)(gamepad->controller);
		if (controller != nil)
		{
			GCExtendedGamepad *extendedGamepad = controller.extendedGamepad;
			if (extendedGamepad != nil)
			{
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_A, extendedGamepad.buttonA, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_B, extendedGamepad.buttonB, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_X, extendedGamepad.buttonX, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_Y, extendedGamepad.buttonY, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_LEFTSHOULDER, extendedGamepad.leftShoulder, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_RIGHTSHOULDER, extendedGamepad.rightShoulder, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_LEFTTRIGGER, extendedGamepad.leftTrigger, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_RIGHTTRIGGER, extendedGamepad.rightTrigger, gamepadManager->eventsBuffer, &eventIndex);
				if (@available(iOS 13.0, macOS 10.15, *))
				{
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_START, extendedGamepad.buttonMenu, gamepadManager->eventsBuffer, &eventIndex);
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_BACK, extendedGamepad.buttonOptions, gamepadManager->eventsBuffer, &eventIndex);
				}
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_UP, extendedGamepad.dpad.up, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_DOWN, extendedGamepad.dpad.down, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_LEFT, extendedGamepad.dpad.left, gamepadManager->eventsBuffer, &eventIndex);
				_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_RIGHT, extendedGamepad.dpad.right, gamepadManager->eventsBuffer, &eventIndex);
			}
			else
			{
				GCMicroGamepad *microGamepad = controller.microGamepad;
				if (microGamepad != nil)
				{
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_A, microGamepad.buttonA, gamepadManager->eventsBuffer, &eventIndex);
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_B, microGamepad.buttonX, gamepadManager->eventsBuffer, &eventIndex);
					if (@available(iOS 13.0, macOS 10.15, *))
					{
						_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_START, microGamepad.buttonMenu, gamepadManager->eventsBuffer, &eventIndex);
					}
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_UP, microGamepad.dpad.up, gamepadManager->eventsBuffer, &eventIndex);
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_DOWN, microGamepad.dpad.down, gamepadManager->eventsBuffer, &eventIndex);
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_LEFT, microGamepad.dpad.left, gamepadManager->eventsBuffer, &eventIndex);
					_addButtonEventIfNeeded(gamepad, GAMEPAD_BUTTON_DPAD_RIGHT, microGamepad.dpad.right, gamepadManager->eventsBuffer, &eventIndex);
				}
			}
		}
	}
	
	*eventCount = eventIndex;
	return gamepadManager->eventsBuffer;
}

const char *GC_NAME(gamepadName)(struct GC_NAME(_GamepadManager) *gamepadManager, GamepadIndex index)
{
	for (uint16_t gamepadIndex = 0; gamepadIndex < MAX_GAMEPADS; gamepadIndex++)
	{
		GC_NAME(Gamepad) *gamepad = &gamepadManager->gamepads[gamepadIndex];
		if (gamepad->index == index)
		{
			return gamepad->name;
		}
	}
	return NULL;
}

#if USE_GC_SPI
bool GC_NAME(hasControllerMatching)(struct GC_NAME(_GamepadManager) *gamepadManager, int32_t vendorID, int32_t productID)
{
	for (uint16_t gamepadIndex = 0; gamepadIndex < MAX_GAMEPADS; gamepadIndex++)
	{
		GC_NAME(Gamepad) *gamepad = &gamepadManager->gamepads[gamepadIndex];
		if (gamepad->vendorID == vendorID && gamepad->productID == productID)
		{
			return true;
		}
	}

	return false;
}
#endif
