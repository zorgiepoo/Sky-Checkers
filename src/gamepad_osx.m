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

#import "gamepad.h"
#import "time.h"
#import <Foundation/Foundation.h>
#import <IOKit/IOKitLib.h>
#import <IOKit/IOCFPlugIn.h>
#import <IOKit/hid/IOHIDLib.h>
#import <IOKit/hid/IOHIDKeys.h>

#define GAMEPAD_EVENT_BUFFER_CAPACITY (MAX_GAMEPADS * GAMEPAD_BUTTON_MAX)

#define AXIS_DEADZONE_THRESHOLD 0.3
#define AXIS_MIN_SCALE (-1.0)
#define AXIS_MAX_SCALE 1.0

typedef struct _Element
{
	IOHIDElementRef hidElement;
	IOHIDElementCookie hidElementCookie;
	uint32_t hidUsage;
	CFIndex hidIndex;
	CFIndex minimum;
	CFIndex maximum;
} Element;

#define INITIAL_ELEMENT_ARRAY_CAPACITY 32
#define ELEMENT_ARRAY_GROWTH_FACTOR 1.6f

typedef struct _ElementArray
{
	Element *element;
	uint16_t count;
	uint16_t capacity;
} ElementArray;

typedef enum
{
	GAMEPAD_AXIS_DIRECTION_UNSPECIFIED,
	GAMEPAD_AXIS_DIRECTION_POSITIVE,
	GAMEPAD_AXIS_DIRECTION_NEGATIVE
} GamepadAxisDirection;

typedef struct
{
	GamepadElementMappingType type;
	CFIndex index;
	union
	{
		CFIndex hatSwitch;
		GamepadAxisDirection axisDirection;
	};
} GamepadElementMapping;

#define GAMEPAD_NAME_SIZE 256

typedef struct _Gamepad
{
	IOHIDDeviceRef device;
	ElementArray axisElements;
	ElementArray buttonElements;
	ElementArray hatElements;
	GamepadState lastStates[GAMEPAD_BUTTON_MAX];
	GamepadElementMapping elementMappings[GAMEPAD_BUTTON_MAX];
	char name[GAMEPAD_NAME_SIZE];
	GamepadIndex index;
} Gamepad;

struct _GamepadManager
{
	IOHIDManagerRef hidManager;
	Gamepad gamepads[MAX_GAMEPADS];
	GamepadEvent eventsBuffer[GAMEPAD_EVENT_BUFFER_CAPACITY];
	CFStringRef database;
	GamepadCallback addedCallback;
	GamepadCallback removalCallback;
	GamepadIndex nextGamepadIndex;
};

static int32_t _gamepadIdentifierInfo(IOHIDDeviceRef device, CFStringRef propertyKey)
{
	CFTypeRef property = IOHIDDeviceGetProperty(device, propertyKey);
	if (property != NULL && CFGetTypeID(property) == CFNumberGetTypeID())
	{
		int32_t value = 0;
		if (!CFNumberGetValue(property, kCFNumberSInt32Type, &value))
		{
			return 0;
		}
		else
		{
			return value;
		}
	}
	return 0;
}

static void _addHIDElement(ElementArray *elementArray, IOHIDElementRef hidElement, IOHIDElementCookie hidElementCookie, uint32_t hidUsage)
{
	// Make sure element isn't already added
	for (uint16_t elementIndex = 0; elementIndex < elementArray->count; elementIndex++)
	{
		if (elementArray->element[elementIndex].hidElementCookie == hidElementCookie)
		{
			return;
		}
	}
	
	// Make room for the new element is needed
	if (elementArray->count == elementArray->capacity)
	{
		uint16_t newCapacity = (elementArray->capacity == 0) ? INITIAL_ELEMENT_ARRAY_CAPACITY : (uint16_t)(elementArray->capacity * ELEMENT_ARRAY_GROWTH_FACTOR);
		
		void *newBuffer = realloc(elementArray->element, newCapacity * sizeof(*elementArray->element));
		if (newBuffer != NULL)
		{
			elementArray->element = newBuffer;
			elementArray->capacity = newCapacity;
		}
	}
	
	if (elementArray->count == elementArray->capacity)
	{
		return;
	}
	
	// Add our new element
	Element *newElement = &elementArray->element[elementArray->count];
	CFRetain(hidElement);
	newElement->hidElement = hidElement;
	newElement->hidElementCookie = hidElementCookie;
	newElement->hidUsage = hidUsage;
	newElement->hidIndex = elementArray->count;
	newElement->minimum = IOHIDElementGetLogicalMin(hidElement);
	newElement->maximum = IOHIDElementGetLogicalMax(hidElement);
	
	elementArray->count++;
}

static void _addHIDElements(Gamepad *gamepad, CFArrayRef hidElements)
{
	for (CFIndex hidElementIndex = 0; hidElementIndex < CFArrayGetCount(hidElements); hidElementIndex++)
	{
		IOHIDElementRef hidElement = (IOHIDElementRef)CFArrayGetValueAtIndex(hidElements, hidElementIndex);
		if (CFGetTypeID(hidElement) != IOHIDElementGetTypeID())
		{
			continue;
		}
		
		IOHIDElementCookie elementCookie = IOHIDElementGetCookie(hidElement);
		
		IOHIDElementType elementType = IOHIDElementGetType(hidElement);
		switch (elementType)
		{
			case kIOHIDElementTypeInput_Axis:
			case kIOHIDElementTypeInput_Button:
			case kIOHIDElementTypeInput_Misc:
			{
				uint32_t usagePage = IOHIDElementGetUsagePage(hidElement);
				uint32_t usage = IOHIDElementGetUsage(hidElement);
				
				switch (usagePage)
				{
					case kHIDPage_GenericDesktop:
					{
						switch (usage)
						{
							case kHIDUsage_GD_X:
                            case kHIDUsage_GD_Y:
                            case kHIDUsage_GD_Z:
                            case kHIDUsage_GD_Rx:
                            case kHIDUsage_GD_Ry:
                            case kHIDUsage_GD_Rz:
                            case kHIDUsage_GD_Slider:
                            case kHIDUsage_GD_Dial:
                            case kHIDUsage_GD_Wheel:
							{
								_addHIDElement(&gamepad->axisElements, hidElement, elementCookie, usage);
								break;
							}
							case kHIDUsage_GD_Hatswitch:
							{
								_addHIDElement(&gamepad->hatElements, hidElement, elementCookie, usage);
								break;
							}
							case kHIDUsage_GD_DPadUp:
                            case kHIDUsage_GD_DPadDown:
                            case kHIDUsage_GD_DPadRight:
                            case kHIDUsage_GD_DPadLeft:
                            case kHIDUsage_GD_Start:
                            case kHIDUsage_GD_Select:
                            case kHIDUsage_GD_SystemMainMenu:
							{
								_addHIDElement(&gamepad->buttonElements, hidElement, elementCookie, usage);
								break;
							}
						}
						break;
					}
					case kHIDPage_Simulation:
					{
						switch (usage)
						{
							case kHIDUsage_Sim_Rudder:
                            case kHIDUsage_Sim_Throttle:
                            case kHIDUsage_Sim_Accelerator:
                            case kHIDUsage_Sim_Brake:
							{
								_addHIDElement(&gamepad->axisElements, hidElement, elementCookie, usage);
								break;
							}
						}
						break;
					}
					
					case kHIDPage_Button:
					case kHIDPage_Consumer:
					{
						_addHIDElement(&gamepad->buttonElements, hidElement, elementCookie, usage);
						break;
					}
				}
				
				break;
			}
			case kIOHIDElementTypeCollection:
			{
				CFArrayRef hidChildrenElements = IOHIDElementGetChildren(hidElement);
				if (hidChildrenElements != NULL)
				{
					_addHIDElements(gamepad, hidChildrenElements);
				}
				break;
			}
			default:
				break;
		}
	}
}

static int _compareElements(const void *element1, const void *element2)
{
	const Element *firstElement = element1;
	const Element *secondElement = element2;
	
	if (firstElement->hidUsage < secondElement->hidUsage)
	{
		return -1;
	}
	else if (firstElement->hidUsage > secondElement->hidUsage)
	{
		return 1;
	}
	else if (firstElement->hidIndex < secondElement->hidIndex)
	{
		return -1;
	}
	else if (firstElement->hidIndex > secondElement->hidIndex)
	{
		return 1;
	}
	return 0;
}

static void _sortGamepadElements(ElementArray *elements)
{
	if (elements->count > 0)
	{
		qsort(elements->element, elements->count, sizeof(*elements->element), _compareElements);
	}
}

static CFIndex _convertSDLHatToDeviceHat(CFIndex hatValue)
{
	switch (hatValue)
	{
		case 1: // up
			return 0;
		case 2: // right
			return 2;
		case 4: // down
			return 4;
		case 8: // left
			return 6;
		case (2 | 1): // right up
			return 1;
		case (2 | 4): // right down
			return 3;
		case (8 | 1): // left up
			return 7;
		case (8 | 4): // left down
			return 5;
		default: // centered or other
			return -1;
	}
}

static NSArray<NSString *> *_gamepadButtonStringMappings(NSArray<NSString *> *mappings)
{
	return [mappings subarrayWithRange:NSMakeRange(2, mappings.count - 2)];
}

static void _hidDeviceMatchingCallback(void *context, IOReturn result, void *sender, IOHIDDeviceRef device)
{
	GamepadManager *gamepadManager = context;
	uint16_t newGamepadIndex = MAX_GAMEPADS;
	for (uint16_t gamepadIndex = 0; gamepadIndex < MAX_GAMEPADS; gamepadIndex++)
	{
		if (gamepadManager->gamepads[gamepadIndex].device == device)
		{
			return;
		}
		
		if (newGamepadIndex == MAX_GAMEPADS && gamepadManager->gamepads[gamepadIndex].device == NULL)
		{
			newGamepadIndex = gamepadIndex;
		}
	}
	
	if (newGamepadIndex == MAX_GAMEPADS)
	{
		fprintf(stderr, "Error: Ignoring gamepad because too many are connected to the system.\n");
		return;
	}
	
	char name[GAMEPAD_NAME_SIZE] = {0};
	CFTypeRef productProperty = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey));
	if (productProperty != NULL && CFGetTypeID(productProperty) == CFStringGetTypeID())
	{
		if (!CFStringGetCString(productProperty, name, sizeof(name), kCFStringEncodingUTF8))
		{
			strncpy(name, "Unknown", sizeof(name));
		}
	}
	else
	{
		strncpy(name, "Unknown", sizeof(name));
	}
	
	int32_t vendorID = _gamepadIdentifierInfo(device, CFSTR(kIOHIDVendorIDKey));
	int32_t productID = _gamepadIdentifierInfo(device, CFSTR(kIOHIDProductIDKey));
	int32_t versionID = _gamepadIdentifierInfo(device, CFSTR(kIOHIDVersionNumberKey));
	
	// Generate a guid matching what SDL uses
	NSString *guidString;
	if (vendorID > 0 && productID > 0)
	{
		guidString = [NSString stringWithFormat:@"03000000%02x%02x0000%02x%02x0000%02x%02x0000", (uint8_t)vendorID, (uint8_t)(vendorID >> 8), (uint8_t)productID, (uint8_t)(productID >> 8), (uint8_t)versionID, (uint8_t)(versionID >> 8)];
	}
	else
	{
		guidString = [NSString stringWithFormat:@"05000000%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x00", name[0], name[1], name[2], name[3], name[4], name[5], name[6], name[7], name[8], name[9], name[10]];
	}
	
	// Try to find our gamepad in the database
	__block NSArray<NSString *> *foundGamepadMapping = nil;
	NSString *gamepadDatabase = (__bridge NSString *)gamepadManager->database;
	[gamepadDatabase enumerateLinesUsingBlock:^(NSString * _Nonnull line, BOOL * _Nonnull stop) {
	 NSString *strippedLine = [line stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
	 
	 if (strippedLine.length > 0 && ![strippedLine hasPrefix:@"#"] && [strippedLine containsString:@"platform:Mac OS X"])
	 {
		NSArray<NSString *> *mappingComponents = [strippedLine componentsSeparatedByString:@","];
		if ([mappingComponents[0] isEqualToString:guidString])
		{
			foundGamepadMapping = mappingComponents;
			*stop = YES;
		}
	 }
	}];
	
	// Check if we can support the gamepad
	if (foundGamepadMapping == nil || foundGamepadMapping.count < 2)
	{
		fprintf(stderr, "Error: Failed to find mapping for controller: %s\n", guidString.UTF8String);
		return;
	}
	
	Gamepad *newGamepad = &gamepadManager->gamepads[newGamepadIndex];
	CFRetain(device);
	newGamepad->device = device;
	newGamepad->index = gamepadManager->nextGamepadIndex;
	gamepadManager->nextGamepadIndex++;
	
	const char *mappingName = foundGamepadMapping[1].UTF8String;
	strncpy(newGamepad->name, mappingName, sizeof(newGamepad->name));
	
	// Create element mappings
	NSDictionary<NSString *, NSNumber *> *buttonStringMappings =
	@{
		@"a" : @(GAMEPAD_BUTTON_A),
		@"b" : @(GAMEPAD_BUTTON_B),
		@"x" : @(GAMEPAD_BUTTON_X),
		@"y" : @(GAMEPAD_BUTTON_Y),
		@"back" : @(GAMEPAD_BUTTON_BACK),
		@"start" : @(GAMEPAD_BUTTON_START),
		@"leftshoulder" : @(GAMEPAD_BUTTON_LEFTSHOULDER),
		@"rightshoulder" : @(GAMEPAD_BUTTON_RIGHTSHOULDER),
		@"lefttrigger" : @(GAMEPAD_BUTTON_LEFTTRIGGER),
		@"righttrigger" : @(GAMEPAD_BUTTON_RIGHTTRIGGER),
		@"dpup" : @(GAMEPAD_BUTTON_DPAD_UP),
		@"dpdown" : @(GAMEPAD_BUTTON_DPAD_DOWN),
		@"dpleft" : @(GAMEPAD_BUTTON_DPAD_LEFT),
		@"dpright" : @(GAMEPAD_BUTTON_DPAD_RIGHT)
	};
	
	NSArray<NSString *> *stringMappings = _gamepadButtonStringMappings(foundGamepadMapping);
	
	for (NSString *stringMapping in stringMappings)
	{
		NSArray<NSString *> *stringMappingComponents = [stringMapping componentsSeparatedByString:@":"];
		if (stringMappingComponents.count != 2) continue;
		
		NSNumber *buttonMappingNumber = [buttonStringMappings objectForKey:stringMappingComponents[0]];
		if (buttonMappingNumber == nil) continue;
		
		GamepadButton buttonMapping = (GamepadButton)buttonMappingNumber.integerValue;
		GamepadElementMapping *elementMapping = &newGamepad->elementMappings[buttonMapping];
		
		NSString *stringMappingValue = stringMappingComponents[1];
		if ([stringMappingValue hasPrefix:@"b"])
		{
			elementMapping->type = GAMEPAD_ELEMENT_MAPPING_TYPE_BUTTON;
			elementMapping->index = [[stringMappingValue substringFromIndex:1] integerValue];
		}
		else if ([stringMappingValue hasPrefix:@"h"])
		{
			elementMapping->type = GAMEPAD_ELEMENT_MAPPING_TYPE_HAT;
			
			NSArray<NSString *> *hatComponents = [[stringMappingValue substringFromIndex:1] componentsSeparatedByString:@"."];
			
			if (hatComponents.count != 2) continue;
			
			elementMapping->index = [hatComponents[0] integerValue];
			elementMapping->hatSwitch = _convertSDLHatToDeviceHat([hatComponents[1] integerValue]);
		}
		else
		{
			BOOL positiveAxis = [stringMappingValue hasPrefix:@"+a"];
			BOOL negativeAxis = [stringMappingValue hasPrefix:@"-a"];
			BOOL unspecifiedAxis = [stringMappingValue hasPrefix:@"a"];
			
			if (positiveAxis || negativeAxis || unspecifiedAxis)
			{
				elementMapping->type = GAMEPAD_ELEMENT_MAPPING_TYPE_AXIS;
				
				if (unspecifiedAxis)
				{
					elementMapping->axisDirection = GAMEPAD_AXIS_DIRECTION_UNSPECIFIED;
				}
				else if (positiveAxis)
				{
					elementMapping->axisDirection = GAMEPAD_AXIS_DIRECTION_POSITIVE;
				}
				else /* if (negativeAxis) */
				{
					elementMapping->axisDirection = GAMEPAD_AXIS_DIRECTION_NEGATIVE;
				}
				
				elementMapping->index = [[stringMappingValue substringFromIndex:unspecifiedAxis ? 1 : 2] integerValue];
			}
		}
	}
	
	// Find what the gamepad supports
	CFArrayRef hidElements = IOHIDDeviceCopyMatchingElements(device, NULL, kIOHIDOptionsTypeNone);
	_addHIDElements(newGamepad, hidElements);
	CFRelease(hidElements);
	
	// Sort gamepad elements based on SDL sorting order
	_sortGamepadElements(&newGamepad->axisElements);
	_sortGamepadElements(&newGamepad->buttonElements);
	_sortGamepadElements(&newGamepad->hatElements);
	
	if (gamepadManager->addedCallback != NULL)
	{
		gamepadManager->addedCallback(newGamepad->index);
	}
}

static void _freeElements(ElementArray *elements)
{
	for (uint16_t elementIndex = 0; elementIndex < elements->count; elementIndex++)
	{
		CFRelease(elements->element[elementIndex].hidElement);
	}
	free(elements->element);
}

static void _hidDeviceRemovalCallback(void *context, IOReturn result, void *sender, IOHIDDeviceRef device)
{
	GamepadManager *gamepadManager = context;
	for (uint16_t gamepadIndex = 0; gamepadIndex < MAX_GAMEPADS; gamepadIndex++)
	{
		if (gamepadManager->gamepads[gamepadIndex].device == device)
		{
			_freeElements(&gamepadManager->gamepads[gamepadIndex].buttonElements);
			_freeElements(&gamepadManager->gamepads[gamepadIndex].hatElements);
			_freeElements(&gamepadManager->gamepads[gamepadIndex].axisElements);
			
			CFRelease(device);
			
			memset(&gamepadManager->gamepads[gamepadIndex], 0, sizeof(gamepadManager->gamepads[gamepadIndex]));
			
			if (gamepadManager->removalCallback != NULL)
			{
				gamepadManager->removalCallback(gamepadIndex);
			}
			
			break;
		}
	}
}

static NSDictionary<NSString *, NSNumber *> *_matchingCriteriaForUsage(NSInteger usage)
{
	return @{@kIOHIDDeviceUsagePageKey : @(kHIDPage_GenericDesktop), @kIOHIDDeviceUsageKey : @(usage)};
}

GamepadManager *initGamepadManager(const char *databasePath, GamepadCallback addedCallback, GamepadCallback removalCallback)
{
	CGEventSourceRef eventSource = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
	CGEventSourceSetLocalEventsSuppressionInterval(eventSource, 0.0);
	
	GamepadManager *gamepadManager = calloc(sizeof(*gamepadManager), 1);
	assert(gamepadManager != NULL);
	
	gamepadManager->addedCallback = addedCallback;
	gamepadManager->removalCallback = removalCallback;
	
	NSError *gamepadDatabaseError = nil;
	NSString *gamepadDatabase = [NSString stringWithContentsOfFile:@(databasePath) encoding:NSUTF8StringEncoding error:&gamepadDatabaseError];
	if (gamepadDatabase == nil)
	{
		fprintf(stderr, "Error: Failed to initialize gamepad database\n");
		abort();
	}
	
	gamepadManager->database = (CFStringRef)CFBridgingRetain(gamepadDatabase);
	
	gamepadManager->hidManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
	assert(gamepadManager->hidManager != NULL);
	
	if (IOHIDManagerOpen(gamepadManager->hidManager, kIOHIDOptionsTypeNone) != kIOReturnSuccess)
	{
		fprintf(stderr, "Error: IOHIDManagerOpen() failed\n");
	}
	
	NSArray<NSDictionary<NSString *, NSNumber *> *> *matchingCriteria =
	@[_matchingCriteriaForUsage(kHIDUsage_GD_Joystick), _matchingCriteriaForUsage(kHIDUsage_GD_GamePad), _matchingCriteriaForUsage(kHIDUsage_GD_MultiAxisController)];
	
	IOHIDManagerSetDeviceMatchingMultiple(gamepadManager->hidManager, (CFArrayRef)matchingCriteria);

	IOHIDManagerRegisterDeviceMatchingCallback(gamepadManager->hidManager, _hidDeviceMatchingCallback, gamepadManager);
	IOHIDManagerRegisterDeviceRemovalCallback(gamepadManager->hidManager, _hidDeviceRemovalCallback, gamepadManager);
	
	IOHIDManagerScheduleWithRunLoop(gamepadManager->hidManager, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
	
	// Run runloop once to detect any gamepads
	CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
	
	return gamepadManager;
}

GamepadEvent *pollGamepadEvents(GamepadManager *gamepadManager, const void *systemEvent, uint16_t *eventCount)
{
	if (systemEvent != NULL)
	{
		*eventCount = 0;
		return NULL;
	}
	
	uint16_t eventIndex = 0;
	for (uint16_t gamepadIndex = 0; gamepadIndex < MAX_GAMEPADS; gamepadIndex++)
	{
		Gamepad *gamepad = &gamepadManager->gamepads[gamepadIndex];
		if (gamepad->device == NULL) continue;
		
		for (GamepadButton gamepadButton = 0; gamepadButton < GAMEPAD_BUTTON_MAX; gamepadButton++)
		{
			GamepadElementMapping *elementMapping = &gamepad->elementMappings[gamepadButton];
			
			switch (elementMapping->type)
			{
				case GAMEPAD_ELEMENT_MAPPING_TYPE_BUTTON:
				{
					CFIndex buttonIndex = elementMapping->index;
					if (buttonIndex >= 0 && buttonIndex < gamepad->buttonElements.count)
					{
						Element *buttonElement = &gamepad->buttonElements.element[buttonIndex];
						IOHIDValueRef buttonValueRef = NULL;
						if (IOHIDDeviceGetValue(gamepad->device, buttonElement->hidElement, &buttonValueRef) == kIOReturnSuccess)
						{
							CFIndex buttonValue = IOHIDValueGetIntegerValue(buttonValueRef);
							GamepadState state = (buttonValue - buttonElement->minimum > 0) ? GAMEPAD_STATE_PRESSED : GAMEPAD_STATE_RELEASED;
							
							if (gamepad->lastStates[gamepadButton] != state)
							{
								gamepad->lastStates[gamepadButton] = state;
								
								GamepadEvent event = {.button = gamepadButton, .index = gamepad->index, .state = state, .mappingType = GAMEPAD_ELEMENT_MAPPING_TYPE_BUTTON, .ticks = ZGGetTicks()};
								
								gamepadManager->eventsBuffer[eventIndex] = event;
								eventIndex++;
							}
						}
					}
					break;
				}
				case GAMEPAD_ELEMENT_MAPPING_TYPE_AXIS:
				{
					CFIndex axisIndex = elementMapping->index;
					if (axisIndex >= 0 && axisIndex < gamepad->axisElements.count)
					{
						Element *axisElement = &gamepad->axisElements.element[axisIndex];
						IOHIDValueRef axisValueRef = NULL;
						if (IOHIDDeviceGetValue(gamepad->device, axisElement->hidElement, &axisValueRef) == kIOReturnSuccess)
						{
							double_t desiredScale = AXIS_MAX_SCALE - AXIS_MIN_SCALE;
							
							double_t hidScaledValue = IOHIDValueGetScaledValue(axisValueRef, kIOHIDValueScaleTypeCalibrated);
							
							double_t scaledValue = (hidScaledValue - (double_t)axisElement->minimum) * desiredScale / ((double_t)axisElement->maximum - (double_t)axisElement->minimum) + AXIS_MIN_SCALE;
							
							GamepadState state;
							switch (elementMapping->axisDirection)
							{
								case GAMEPAD_AXIS_DIRECTION_UNSPECIFIED:
									state = (fabs(scaledValue) > AXIS_DEADZONE_THRESHOLD) ? GAMEPAD_STATE_PRESSED : GAMEPAD_STATE_RELEASED;
									break;
								case GAMEPAD_AXIS_DIRECTION_POSITIVE:
									state = (scaledValue > AXIS_DEADZONE_THRESHOLD);
									break;
								case GAMEPAD_AXIS_DIRECTION_NEGATIVE:
									state = (scaledValue < -AXIS_DEADZONE_THRESHOLD);
									break;
							}
							
							if (gamepad->lastStates[gamepadButton] != state)
							{
								gamepad->lastStates[gamepadButton] = state;
								
								GamepadEvent event = {.button = gamepadButton, .index = gamepad->index, .state = state, .mappingType = GAMEPAD_ELEMENT_MAPPING_TYPE_AXIS, .ticks = ZGGetTicks()};
								
								gamepadManager->eventsBuffer[eventIndex] = event;
								eventIndex++;
							}
						}
					}
					break;
				}
				case GAMEPAD_ELEMENT_MAPPING_TYPE_HAT:
				{
					CFIndex hatIndex = elementMapping->index;
					if (hatIndex >= 0 && hatIndex < gamepad->hatElements.count)
					{
						Element *hatElement = &gamepad->hatElements.element[hatIndex];
						IOHIDValueRef hatValueRef = NULL;
						if (IOHIDDeviceGetValue(gamepad->device, hatElement->hidElement, &hatValueRef) == kIOReturnSuccess)
						{
							CFIndex hatValue = IOHIDValueGetIntegerValue(hatValueRef);
							CFIndex hatRange = hatElement->maximum - hatElement->minimum + 1;
							
							CFIndex scaledHatValue;
							switch (hatRange)
							{
								case 4:
									scaledHatValue = (hatValue - hatElement->minimum) * 2;
									break;
								case 8:
									scaledHatValue = (hatValue - hatElement->minimum);
									break;
								default:
									continue;
							}
							
							GamepadState state = (elementMapping->hatSwitch == scaledHatValue) ? GAMEPAD_STATE_PRESSED : GAMEPAD_STATE_RELEASED;
							
							if (gamepad->lastStates[gamepadButton] != state)
							{
								gamepad->lastStates[gamepadButton] = state;
								
								GamepadEvent event = {.button = gamepadButton, .index = gamepad->index, .state = state, .mappingType = GAMEPAD_ELEMENT_MAPPING_TYPE_HAT, .ticks = ZGGetTicks()};
								
								gamepadManager->eventsBuffer[eventIndex] = event;
								eventIndex++;
							}
						}
					}
					break;
				}
				case GAMEPAD_ELEMENT_MAPPING_TYPE_INVALID:
					break;
			}
		}
	}
	
	*eventCount = eventIndex;
	return gamepadManager->eventsBuffer;
}
