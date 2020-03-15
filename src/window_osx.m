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

#import "window.h"
#import "quit.h"
#import "zgtime.h"

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

@interface ZGGameView : NSView

@property (nonatomic) void (*keyboardEventHandler)(ZGKeyboardEvent, void *);
@property (nonatomic) void *keyboardEventHandlerContext;

@end

@implementation ZGGameView
{
	NSCursor *_invisibleCursor;
}

- (instancetype)initWithFrame:(NSRect)frame
{
	self = [super initWithFrame:frame];
	if (self != nil)
	{
		self.wantsLayer = YES;
		_invisibleCursor = [self zgCreateInvisibleCursor];
	}
	return self;
}

- (BOOL)wantsUpdateLayer
{
    return YES;
}

- (void)updateLayer
{
	// Initial background color of our window should be black instead of default gray window gradient
    self.layer.backgroundColor = CGColorGetConstantColor(kCGColorBlack);
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)keyDown:(NSEvent *)theEvent
{
	if (_keyboardEventHandler != NULL)
	{
		ZGKeyboardEvent event;
		event.type = ZGKeyboardEventTypeKeyDown;
		event.keyCode = theEvent.keyCode;
		event.keyModifier = theEvent.modifierFlags;
		event.timestamp = ZGGetNanoTicks();
		
		_keyboardEventHandler(event, _keyboardEventHandlerContext);
		
		[self interpretKeyEvents:@[theEvent]];
	}
}

- (void)keyUp:(NSEvent *)theEvent
{
	if (_keyboardEventHandler != NULL)
	{
		ZGKeyboardEvent event;
		event.type = ZGKeyboardEventTypeKeyUp;
		event.keyCode = theEvent.keyCode;
		event.keyModifier = theEvent.modifierFlags;
		event.timestamp = ZGGetNanoTicks();
		
		_keyboardEventHandler(event, _keyboardEventHandlerContext);
	}
}

- (void)insertText:(id)stringObject
{
	if (_keyboardEventHandler != NULL)
	{
		ZGKeyboardEvent event = {};
		event.type = ZGKeyboardEventTypeTextInput;
		
		NSString *string;
		if ([stringObject isKindOfClass:[NSAttributedString class]])
		{
			string = [(NSAttributedString *)string string];
		}
		else if ([stringObject isKindOfClass:[NSString class]])
		{
			string = stringObject;
		}
		else
		{
			return;
		}
		
		static NSCharacterSet *badCharacterSet;
		if (badCharacterSet == nil)
		{
			NSMutableCharacterSet *goodCharacterSet = [NSMutableCharacterSet characterSetWithCharactersInString:@" .:-"];
			[goodCharacterSet formUnionWithCharacterSet:[NSCharacterSet alphanumericCharacterSet]];
			badCharacterSet = [goodCharacterSet invertedSet];
		}
		
		NSString *strippedString = [string stringByTrimmingCharactersInSet:badCharacterSet];
		if (strippedString.length == 0)
		{
			return;
		}
		
		const char *utf8String = [strippedString UTF8String];
		if (utf8String == NULL)
		{
			return;
		}
		
		strncpy(event.text, utf8String, sizeof(event.text) / sizeof(event.text[0]) - 1);
		
		_keyboardEventHandler(event, _keyboardEventHandlerContext);
	}
}

- (void)doCommandBySelector:(SEL)selector
{
}

/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2019 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
 
 Code in this method has been taken from SDL as noted above,
 with some slight altered formatting & scope changes.
*/
- (NSCursor *)zgCreateInvisibleCursor
{
	// RAW 16x16 transparent GIF
	static unsigned char cursorBytes[] =
	{
		0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0x10, 0x00, 0x10, 0x00, 0x80,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0xF9, 0x04,
		0x01, 0x00, 0x00, 0x01, 0x00, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x10,
		0x00, 0x10, 0x00, 0x00, 0x02, 0x0E, 0x8C, 0x8F, 0xA9, 0xCB, 0xED,
		0x0F, 0xA3, 0x9C, 0xB4, 0xDA, 0x8B, 0xB3, 0x3E, 0x05, 0x00, 0x3B
	};

	NSData *cursorData = [NSData dataWithBytesNoCopy:cursorBytes length:sizeof(cursorBytes) freeWhenDone:NO];
	NSImage *cursorImage = [[NSImage alloc] initWithData:cursorData];
	return [[NSCursor alloc] initWithImage:cursorImage hotSpot:NSZeroPoint];
}

- (void)resetCursorRects
{
	if (_invisibleCursor != nil)
	{
		// This hides the mouse cursor only when it is within our view bounds which is nice
		[self addCursorRect:[self bounds] cursor:_invisibleCursor];
	}
}

@end

#define AUTOSAVE_FRAME_NAME @"GameWindow"

@interface ZGGameWindowController : NSWindowController <NSWindowDelegate>

@property (nonatomic) void (*windowEventHandler)(ZGWindowEvent, void *);
@property (nonatomic) void *windowEventHandlerContext;

@end

@implementation ZGGameWindowController
{
	bool *_fullscreenFlag;
	bool _needsUnhideCursorInFullscreen;
}

- (instancetype)initWithWindow:(NSWindow *)window fullscreenFlag:(bool *)fullscreenFlag
{
	self = [super initWithWindow:window];
	if (self != nil)
	{
		window.delegate = self;
		_fullscreenFlag = fullscreenFlag;
		
		if (fullscreenFlag != NULL && *fullscreenFlag)
		{
			_needsUnhideCursorInFullscreen = true;
			[NSCursor hide];
			[window toggleFullScreen:nil];
		}
	}
	return self;
}

- (void)windowDidEnterFullScreen:(NSNotification *)notification
{
	if (_needsUnhideCursorInFullscreen)
	{
		_needsUnhideCursorInFullscreen = false;
		dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
			[NSCursor unhide];
		});
	}
	
	if (_fullscreenFlag != NULL)
	{
		*_fullscreenFlag = true;
	}
}

- (void)windowDidExitFullScreen:(NSNotification *)notification
{
	[self.window invalidateCursorRectsForView:self.window.contentView];
	
	if (_fullscreenFlag != NULL)
	{
		*_fullscreenFlag = false;
	}
}

- (void)windowDidMove:(NSNotification *)notification
{
	[self.window saveFrameUsingName:AUTOSAVE_FRAME_NAME];
}

- (void)windowDidResize:(NSNotification *)notification
{
	[self.window saveFrameUsingName:AUTOSAVE_FRAME_NAME];
	
	if (_windowEventHandler != NULL)
	{
		ZGWindowEvent event;
		event.type = ZGWindowEventTypeResize;
		
		NSRect frame = self.window.frame;
		event.width = (int32_t)frame.size.width;
		event.height = (int32_t)frame.size.height;
		
		_windowEventHandler(event, _windowEventHandlerContext);
	}
}

- (void)windowDidBecomeKey:(NSNotification *)notification
{
	if (_windowEventHandler != NULL)
	{
		ZGWindowEvent event;
		event.type = ZGWindowEventTypeFocusGained;
		_windowEventHandler(event, _windowEventHandlerContext);
	}
}

- (void)windowDidResignKey:(NSNotification *)notification
{
	if (_windowEventHandler != NULL)
	{
		ZGWindowEvent event;
		event.type = ZGWindowEventTypeFocusLost;
		_windowEventHandler(event, _windowEventHandlerContext);
	}
}

- (void)windowDidChangeOcclusionState:(NSNotification *)notification
{
	bool occluded = ([self.window occlusionState] & NSWindowOcclusionStateVisible) == 0;
	if (occluded)
	{
		ZGWindowEvent event;
		event.type = ZGWindowEventTypeHidden;
		_windowEventHandler(event, _windowEventHandlerContext);
	}
	else
	{
		ZGWindowEvent event;
		event.type = ZGWindowEventTypeShown;
		_windowEventHandler(event, _windowEventHandlerContext);
	}
}

- (void)windowWillClose:(NSNotification *)notification
{
	ZGSendQuitEvent();
}

@end

ZGWindow *ZGCreateWindow(const char *windowTitle, int32_t windowWidth, int32_t windowHeight, bool *fullscreenFlag)
{
	NSWindowStyleMask styleMask = (NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable);
	
	NSScreen *mainScreen = [NSScreen mainScreen];
	NSRect screenFrame = mainScreen.visibleFrame;
	
	NSWindow *window = [[NSWindow alloc] initWithContentRect:NSMakeRect(screenFrame.origin.x + screenFrame.size.width / 2.0 - windowWidth / 2.0, screenFrame.origin.y + screenFrame.size.height / 2.0 - windowHeight / 2.0, (CGFloat)windowWidth, (CGFloat)windowHeight) styleMask:styleMask backing:NSBackingStoreBuffered defer:NO];
	
	[window setRestorable:NO];
	[window setTitle:@(windowTitle)];
	
	[window setFrameUsingName:AUTOSAVE_FRAME_NAME];
	if ([window respondsToSelector:@selector(setTabbingMode:)])
	{
		[window setTabbingMode:NSWindowTabbingModeDisallowed];
	}
	
	ZGGameView *contentView = [[ZGGameView alloc] initWithFrame:window.frame];
	window.contentView = contentView;
	
	ZGGameWindowController *windowController = [[ZGGameWindowController alloc] initWithWindow:window fullscreenFlag:fullscreenFlag];
	[windowController showWindow:nil];
	
	return (ZGWindow *)CFBridgingRetain(windowController);
}

void ZGDestroyWindow(ZGWindow *window)
{
	CFRelease(window);
}

bool ZGWindowHasFocus(ZGWindow *window)
{
	ZGGameWindowController *windowController = (__bridge ZGGameWindowController *)(window);
	return [windowController.window isKeyWindow];
}

void ZGSetWindowEventHandler(ZGWindow *window, void *context, void (*windowEventHandler)(ZGWindowEvent, void *))
{
	ZGGameWindowController *windowController = (__bridge ZGGameWindowController *)(window);
	windowController.windowEventHandler = windowEventHandler;
	windowController.windowEventHandlerContext = context;
}

void ZGSetKeyboardEventHandler(ZGWindow *window, void *context, void (*keyboardEventHandler)(ZGKeyboardEvent, void *))
{
	ZGGameWindowController *windowController = (__bridge ZGGameWindowController *)(window);
	ZGGameView *gameView = (ZGGameView *)windowController.window.contentView;
	gameView.keyboardEventHandler = keyboardEventHandler;
	gameView.keyboardEventHandlerContext = context;
}

void ZGPollWindowAndInputEvents(ZGWindow *window, const void *systemEvent)
{
	// No need to do anything for macOS implementation
}

void ZGGetWindowSize(ZGWindow *windowRef, int32_t *width, int32_t *height)
{
	ZGGameWindowController *windowController = (__bridge ZGGameWindowController *)(windowRef);
	NSWindow *window = windowController.window;
	
	if (width != NULL)
	{
		*width = (int32_t)window.frame.size.width;
	}
	
	if (height != NULL)
	{
		*height = (int32_t)window.frame.size.height;
	}
}

void *ZGWindowHandle(ZGWindow *window)
{
	ZGGameWindowController *windowController = (__bridge ZGGameWindowController *)(window);
	return (__bridge void *)(windowController.window);
}
