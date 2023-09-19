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

#import "app.h"
#import "zgtime.h"

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <IOKit/pwr_mgt/IOPMLib.h>
#import <QuartzCore/QuartzCore.h>

@interface ZGAppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation ZGAppDelegate
{
	ZGAppHandlers *_appHandlers;
	void *_appContext;
	id _displayTimer;
}

- (instancetype)initWithAppHandlers:(ZGAppHandlers *)appHandlers context:(void *)appContext
{
	self = [super init];
	if (self != nil)
	{
		_appContext = appContext;
		_appHandlers = appHandlers;
	}
	return self;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
	NSString *resourcePath = [[NSBundle mainBundle] resourcePath];
	NSFileManager *fileManager = [NSFileManager defaultManager];
	if (![fileManager changeCurrentDirectoryPath:resourcePath])
	{
		fprintf(stderr, "Error: failed to change current working directory to: %s\n", resourcePath.fileSystemRepresentation);
		abort();
	}
	
	// Initialize time
	(void)ZGGetNanoTicks();
	
	ZGWindow *window;
	if (_appHandlers->launchedHandler != NULL)
	{
		window = _appHandlers->launchedHandler(_appContext);
	}
	else
	{
		window = NULL;
		assert(false);
	}
	
	if (_appHandlers->runLoopHandler != NULL)
	{
		// Game should animate in common run loop mode
		// One instance of this mode being used is when the user is navigating through the menu items in the menu bar
		if (@available(macOS 14, *))
		{
			NSWindow *windowHandle = (__bridge NSWindow *)(ZGWindowHandle(window));
			CADisplayLink *displayLink = [windowHandle.contentView displayLinkWithTarget:self selector:@selector(runLoop:)];
			
			[displayLink addToRunLoop:NSRunLoop.currentRunLoop forMode:NSRunLoopCommonModes];
			
			_displayTimer = displayLink;
		}
		else
		{
			_displayTimer = [NSTimer scheduledTimerWithTimeInterval:0.004 target:self selector:@selector(runLoop:) userInfo:nil repeats:YES];
			
			[[NSRunLoop currentRunLoop] addTimer:_displayTimer forMode:NSRunLoopCommonModes];
		}
	}
}

- (void)runLoop:(id)displayTimer
{
	_appHandlers->pollEventHandler(_appContext, NULL);
	_appHandlers->runLoopHandler(_appContext);
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
	[_displayTimer invalidate];
	_displayTimer = nil;
	
	if (_appHandlers->terminatedHandler != NULL)
	{
		_appHandlers->terminatedHandler(_appContext);
	}
}

@end

int ZGAppInit(int argc, char *argv[], ZGAppHandlers *appHandlers, void *appContext)
{
	@autoreleasepool
	{
		NSApplication *application = [NSApplication sharedApplication];
		ZGAppDelegate *appDelegate = [[ZGAppDelegate alloc] initWithAppHandlers:appHandlers context:appContext];
		application.delegate = appDelegate;
		
		return NSApplicationMain(argc, (const char **)argv);
	}
}

void ZGAppSetAllowsScreenIdling(bool allowsScreenIdling)
{
	static IOPMAssertionID assertionID;
	static bool hasLiveAssertion = false;
	
	if (!allowsScreenIdling)
	{
		if (!hasLiveAssertion)
		{
			IOReturn result = IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep, kIOPMAssertionLevelOn, CFSTR("Gameplay"), &assertionID);
			
			if (result == kIOReturnSuccess)
			{
				hasLiveAssertion = true;
			}
		}
	}
	else
	{
		if (hasLiveAssertion)
		{
			IOPMAssertionRelease(assertionID);
			hasLiveAssertion = false;
		}
	}
}
