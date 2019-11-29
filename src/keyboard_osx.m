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

#import "keyboard.h"

#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>
#import <Carbon/Carbon.h>

static char gKeyCodeNameBuffer[256];
static NSMutableDictionary *gMappingTable;
static TISInputSourceRef gCurrentKeyboard;

bool ZGTestReturnKeyCode(uint16_t keyCode)
{
	const uint16_t returnKeyCode = 36;
	const uint16_t functionReturnKeyCode = 76;
	
	return (keyCode == returnKeyCode || keyCode == functionReturnKeyCode);
}

bool ZGTestMetaModifier(uint64_t modifier)
{
	NSEventModifierFlags modifierFlags = modifier;
	return (modifierFlags & NSEventModifierFlagCommand) != 0;
}

// UCKeyTranslate() doesn't give a printable representation for these keys
const char *ZGGetKnownKeyCodeNameMapping(uint16_t keyCode)
{
	switch (keyCode)
	{
		case ZG_KEYCODE_RIGHT:
			return "right";
		case ZG_KEYCODE_LEFT:
			return "left";
		case ZG_KEYCODE_DOWN:
			return "down";
		case ZG_KEYCODE_UP:
			return "up";
		case ZG_KEYCODE_SPACE:
			return "space";
		default:
			return NULL;
	}
}

const char *ZGGetKeyCodeName(uint16_t keyCode)
{
	const char *knownMapping = ZGGetKnownKeyCodeNameMapping(keyCode);
	if (knownMapping != NULL)
	{
		return knownMapping;
	}
	
	NSNumber *keyCodeNumber = @(keyCode);
	if (gMappingTable != nil)
	{
		NSString *existingMapping = [gMappingTable objectForKey:keyCodeNumber];
		if (existingMapping != nil)
		{
			const char *utf8String = [existingMapping UTF8String];
			if (utf8String != NULL)
			{
				strncpy(gKeyCodeNameBuffer, utf8String, sizeof(gKeyCodeNameBuffer) / sizeof(*gKeyCodeNameBuffer));
				return gKeyCodeNameBuffer;
			}
		}
	}
	
	if (gCurrentKeyboard == NULL)
	{
		// Let us assume that the current keyboard doesn't change during our game's lifetime for now
		gCurrentKeyboard = TISCopyCurrentKeyboardInputSource();
	}
	
	CFDataRef uchrDataRef = TISGetInputSourceProperty(gCurrentKeyboard, kTISPropertyUnicodeKeyLayoutData);
	if (uchrDataRef == NULL)
	{
		return NULL;
	}
	
	const UCKeyboardLayout *keyboardLayout = (UCKeyboardLayout *)CFDataGetBytePtr(uchrDataRef);
	UInt32 deadKeyState = 0;
	UniChar unicodeString[8];
	UniCharCount actualStringLength = 0;
	
	OSStatus status = UCKeyTranslate(keyboardLayout, keyCode, kUCKeyActionDown, 0, LMGetKbdType(), kUCKeyTranslateNoDeadKeysMask, &deadKeyState, sizeof(unicodeString) / sizeof(*unicodeString), &actualStringLength, unicodeString);
	
	if (status != noErr)
	{
		return NULL;
	}
	
	// Make sure we strip out all weird characters. This is necessary in some cases.
	NSString *unicodeStringContents = [[NSString stringWithCharacters:unicodeString length:actualStringLength] stringByTrimmingCharactersInSet:[[NSCharacterSet alphanumericCharacterSet] invertedSet]];
	const char *utf8String = [unicodeStringContents UTF8String];
	
	if (utf8String == NULL)
	{
		return NULL;
	}
	
	strncpy(gKeyCodeNameBuffer, utf8String, sizeof(gKeyCodeNameBuffer) / sizeof(*gKeyCodeNameBuffer));
	
	if (gMappingTable == nil)
	{
		gMappingTable = [[NSMutableDictionary alloc] init];
	}
	
	[gMappingTable setObject:unicodeStringContents forKey:keyCodeNumber];
	
	return gKeyCodeNameBuffer;
}

char *ZGGetClipboardText(void)
{
	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
	NSString *string = [pasteboard stringForType:NSPasteboardTypeString];
	if (string.length == 0)
	{
		return NULL;
	}
	
	size_t bufferCount = 128;
	char *buffer = calloc(128, sizeof(*buffer));
	if (buffer == NULL)
	{
		return NULL;
	}
	
	if (![string getCString:buffer maxLength:bufferCount - 1 encoding:NSUTF8StringEncoding])
	{
		free(buffer);
		return NULL;
	}
	
	return buffer;
}

void ZGFreeClipboardText(char *clipboardText)
{
	free(clipboardText);
}
