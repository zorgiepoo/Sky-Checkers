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
#import <Cocoa/Cocoa.h>
#import <IOKit/pwr_mgt/IOPMLib.h>

@interface ZGAppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation ZGAppDelegate
{
	void (*_appLaunchedHandler)(void *);
	void (*_appTerminatedHandler)(void *);
	void (*_runLoopHandler)(void *);
	void (*_pollEventHandler)(void *, void *);
	
	void *_appContext;
	NSTimer *_timer;
}

- (instancetype)initWithAppContext:(void *)appContext launchedHandler:(void (*)(void *))appLaunchedHandler terminatedHandler:(void (*)(void *))appTerminatedHandler runLoopHandler:(void (*)(void *))runLoopHandler pollEventHandler:(void (*)(void *, void *))pollEventHandler
{
	self = [super init];
	if (self != nil)
	{
		_appContext = appContext;
		_appLaunchedHandler = appLaunchedHandler;
		_appTerminatedHandler = appTerminatedHandler;
		_pollEventHandler = pollEventHandler;
		_runLoopHandler = runLoopHandler;
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
	
	if (_appLaunchedHandler != NULL)
	{
		_appLaunchedHandler(_appContext);
	}
	
	if (_runLoopHandler != NULL)
	{
		_timer = [NSTimer scheduledTimerWithTimeInterval:0.004 target:self selector:@selector(runLoop:) userInfo:nil repeats:YES];
		
		// Game should animate in common run loop mode
		// One instance of this mode being used is when the user is navigating through the menu items in the menu bar
		[[NSRunLoop currentRunLoop] addTimer:_timer forMode:NSRunLoopCommonModes];
	}
}

- (void)runLoop:(NSTimer *)timer
{
	_pollEventHandler(_appContext, NULL);
	_runLoopHandler(_appContext);
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
	[_timer invalidate];
	_timer = nil;
	
	if (_appTerminatedHandler != NULL)
	{
		_appTerminatedHandler(_appContext);
	}
}

@end

int ZGAppInit(int argc, char *argv[], void *appContext, void (*appLaunchedHandler)(void *), void (*appTerminatedHandler)(void *), void (*runLoopHandler)(void *), void (*pollEventHandler)(void *, void *))
{
	@autoreleasepool
	{
		NSApplication *application = [NSApplication sharedApplication];
		ZGAppDelegate *appDelegate = [[ZGAppDelegate alloc] initWithAppContext:appContext launchedHandler:appLaunchedHandler terminatedHandler:appTerminatedHandler runLoopHandler:runLoopHandler pollEventHandler:pollEventHandler];
		application.delegate = appDelegate;
		
		return NSApplicationMain(argc, (const char **)argv);
	}
}

void ZGAppSetAllowsScreenSaver(bool allowsScreenSaver)
{
	static IOPMAssertionID assertionID;
	static bool hasLiveAssertion = false;
	
	if (!allowsScreenSaver)
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
