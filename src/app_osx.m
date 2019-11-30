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

@interface NSApplication (Private)

- (void)setAppleMenu:(NSMenu *)appleMenu;

@end

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
	if (_appLaunchedHandler != NULL)
	{
		_appLaunchedHandler(_appContext);
	}
	
	if (_runLoopHandler != NULL)
	{
		_timer = [NSTimer scheduledTimerWithTimeInterval:0.0 target:self selector:@selector(runLoop:) userInfo:nil repeats:YES];
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

static void setUpCurrentWorkingDirectory(NSBundle *mainBundle)
{
	NSString *resourcePath = [mainBundle resourcePath];
	
	NSFileManager *fileManager = [NSFileManager defaultManager];
	if (![fileManager changeCurrentDirectoryPath:resourcePath])
	{
		fprintf(stderr, "Error: failed to change current working directory to: %s\n", resourcePath.fileSystemRepresentation);
		abort();
	}
}

// TODO: Just use a nib file instead of this mess
// This menu bar layout is borrowed from SDL
static void setUpMenuBar(NSApplication *application, NSBundle *mainBundle)
{
	NSMenu *mainMenu = [[NSMenu alloc] init];
	
	[application setMainMenu:mainMenu];
	
	NSMenu *appleMenu = [[NSMenu alloc] initWithTitle:@""];
	NSString *appName = [mainBundle objectForInfoDictionaryKey:@"CFBundleName"];
	
	[appleMenu addItemWithTitle:[NSString stringWithFormat:@"About %@", appName] action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];
	
	[appleMenu addItem:[NSMenuItem separatorItem]];
	
	[appleMenu addItemWithTitle:@"Preferences…" action:nil keyEquivalent:@","];
	
	[appleMenu addItem:[NSMenuItem separatorItem]];
	
	NSMenu *serviceMenu = [[NSMenu alloc] initWithTitle:@""];
    NSMenuItem *servicesMenuItem = [appleMenu addItemWithTitle:@"Services" action:nil keyEquivalent:@""];
    [servicesMenuItem setSubmenu:serviceMenu];
	
	[appleMenu addItem:[NSMenuItem separatorItem]];
	
	[appleMenu addItemWithTitle:[NSString stringWithFormat:@"Hide %@", appName] action:@selector(hide:) keyEquivalent:@"h"];
	
	NSMenuItem *hideOthersMenuItem = [appleMenu addItemWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h"];
    [hideOthersMenuItem setKeyEquivalentModifierMask:(NSEventModifierFlagOption | NSEventModifierFlagCommand)];
	
	[appleMenu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];
	
	[appleMenu addItem:[NSMenuItem separatorItem]];
	
	[appleMenu addItemWithTitle:[NSString stringWithFormat:@"Quit %@", appName] action:@selector(terminate:) keyEquivalent:@"q"];
	
	NSMenuItem *menuItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    [menuItem setSubmenu:appleMenu];
    [mainMenu addItem:menuItem];
	
	[application setAppleMenu:appleMenu];
	
	NSMenu *windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];
	[windowMenu addItemWithTitle:@"Close" action:@selector(performClose:) keyEquivalent:@"w"];
	[windowMenu addItemWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"];
	[windowMenu addItemWithTitle:@"Zoom" action:@selector(performZoom:) keyEquivalent:@""];
	
	NSMenuItem *toggleFullScreenMenuItem = [[NSMenuItem alloc] initWithTitle:@"Toggle Full Screen" action:@selector(toggleFullScreen:) keyEquivalent:@"f"];
	[toggleFullScreenMenuItem setKeyEquivalentModifierMask:NSEventModifierFlagControl | NSEventModifierFlagCommand];
	[windowMenu addItem:toggleFullScreenMenuItem];
	
	NSMenuItem *windowMenuItem = [[NSMenuItem alloc] initWithTitle:@"Window" action:nil keyEquivalent:@""];
    [windowMenuItem setSubmenu:windowMenu];
    [mainMenu addItem:windowMenuItem];
	
	[application setWindowsMenu:windowMenu];
}

int ZGAppInit(int argc, char *argv[], void *appContext, void (*appLaunchedHandler)(void *), void (*appTerminatedHandler)(void *), void (*runLoopHandler)(void *), void (*pollEventHandler)(void *, void *))
{
	@autoreleasepool
	{
		NSApplication *application = [NSApplication sharedApplication];
		ZGAppDelegate *appDelegate = [[ZGAppDelegate alloc] initWithAppContext:appContext launchedHandler:appLaunchedHandler terminatedHandler:appTerminatedHandler runLoopHandler:runLoopHandler pollEventHandler:pollEventHandler];
		application.delegate = appDelegate;
		
		NSBundle *mainBundle = [NSBundle mainBundle];
		
		setUpCurrentWorkingDirectory(mainBundle);
		setUpMenuBar(application, mainBundle);
		
		[application run];
	}
	return 0;
}
