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
#import <UIKit/UIKit.h>

typedef struct
{
	void *context;
	void (*launchedHandler)(void *);
	void (*terminatedHandler)(void *);
	void (*runLoopHandler)(void *);
	void (*pollEventHandler)(void *, void *);
} ZGAppCallbacks;

static ZGAppCallbacks gAppCallbacks;

@interface ZGAppDelegate : UIResponder <UIApplicationDelegate>
@end

@implementation ZGAppDelegate
{
	NSTimer *_timer;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	NSString *resourcePath = [[NSBundle mainBundle] resourcePath];
	NSFileManager *fileManager = [NSFileManager defaultManager];
	if (![fileManager changeCurrentDirectoryPath:resourcePath])
	{
		fprintf(stderr, "Error: failed to change current working directory to: %s\n", resourcePath.fileSystemRepresentation);
	}
	
	if (gAppCallbacks.launchedHandler != NULL)
	{
		gAppCallbacks.launchedHandler(gAppCallbacks.context);
	}
	
	if (gAppCallbacks.runLoopHandler != NULL)
	{
		_timer = [NSTimer scheduledTimerWithTimeInterval:0.004 target:self selector:@selector(runLoop:) userInfo:nil repeats:YES];
		[[NSRunLoop currentRunLoop] addTimer:_timer forMode:NSRunLoopCommonModes];
	}
	
	return YES;
}

- (void)applicationWillTerminate:(UIApplication *)application
{
	[_timer invalidate];
	_timer = nil;
	
	if (gAppCallbacks.terminatedHandler != NULL)
	{
		gAppCallbacks.terminatedHandler(gAppCallbacks.context);
	}
}

- (void)runLoop:(NSTimer *)timer
{
	gAppCallbacks.pollEventHandler(gAppCallbacks.context, NULL);
	gAppCallbacks.runLoopHandler(gAppCallbacks.context);
}

@end

int ZGAppInit(int argc, char *argv[], void *appContext, void (*appLaunchedHandler)(void *), void (*appTerminatedHandler)(void *), void (*runLoopHandler)(void *), void (*pollEventHandler)(void *, void *))
{
	@autoreleasepool
	{
		gAppCallbacks.context = appContext;
		gAppCallbacks.launchedHandler = appLaunchedHandler;
		gAppCallbacks.terminatedHandler = appTerminatedHandler;
		gAppCallbacks.runLoopHandler = runLoopHandler;
		gAppCallbacks.pollEventHandler = pollEventHandler;
		
		return UIApplicationMain(argc, argv, nil, NSStringFromClass([ZGAppDelegate class]));
	}
}

void ZGAppSetAllowsScreenSaver(bool allowsScreenSaver)
{
	UIApplication *application = [UIApplication sharedApplication];
	[application setIdleTimerDisabled:!allowsScreenSaver];
}
