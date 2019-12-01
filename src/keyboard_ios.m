//
//  keyboard_ios.m
//  SkyCheckers-iOS
//
//  Created by Mayur Pawashe on 12/1/19.
//

#import "keyboard.h"
#import <Foundation/Foundation.h>

bool ZGTestReturnKeyCode(uint16_t keyCode)
{
	return false;
}

bool ZGTestMetaModifier(uint64_t modifier)
{
	return false;
}

const char *ZGGetKeyCodeName(uint16_t keyCode)
{
	return NULL;
}

char *ZGGetClipboardText(void)
{
	return NULL;
}

void ZGFreeClipboardText(char *clipboardText)
{
}
