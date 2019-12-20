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
	ZGAppHandlers *appHandlers;
	void *context;
} ZGAppCallbacks;

static ZGAppCallbacks gAppCallbacks;

@interface ZGAppDelegate : UIResponder <UIApplicationDelegate>
@end

@implementation ZGAppDelegate
{
	CADisplayLink *_displayLink;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	NSString *resourcePath = [[NSBundle mainBundle] resourcePath];
	NSFileManager *fileManager = [NSFileManager defaultManager];
	if (![fileManager changeCurrentDirectoryPath:resourcePath])
	{
		fprintf(stderr, "Error: failed to change current working directory to: %s\n", resourcePath.fileSystemRepresentation);
		abort();
	}
	
	if (gAppCallbacks.appHandlers->launchedHandler != NULL)
	{
		gAppCallbacks.appHandlers->launchedHandler(gAppCallbacks.context);
	}
	
	if (gAppCallbacks.appHandlers->runLoopHandler != NULL)
	{
		_displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(runLoop:)];
		[_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
	}
	
	return YES;
}

- (void)applicationWillTerminate:(UIApplication *)application
{
	[_displayLink invalidate];
	_displayLink = nil;
	
	if (gAppCallbacks.appHandlers->terminatedHandler != NULL)
	{
		gAppCallbacks.appHandlers->terminatedHandler(gAppCallbacks.context);
	}
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
	if (gAppCallbacks.appHandlers->suspendedHandler != NULL)
	{
		gAppCallbacks.appHandlers->suspendedHandler(gAppCallbacks.context);
	}
}

- (void)runLoop:(CADisplayLink *)displayLink
{
	gAppCallbacks.appHandlers->pollEventHandler(gAppCallbacks.context, NULL);
	gAppCallbacks.appHandlers->runLoopHandler(gAppCallbacks.context);
}

@end

int ZGAppInit(int argc, char *argv[], ZGAppHandlers *appHandlers, void *appContext)
{
	@autoreleasepool
	{
		gAppCallbacks.context = appContext;
		gAppCallbacks.appHandlers = appHandlers;
		
		return UIApplicationMain(argc, argv, nil, NSStringFromClass([ZGAppDelegate class]));
	}
}

void ZGAppSetAllowsScreenSaver(bool allowsScreenSaver)
{
	UIApplication *application = [UIApplication sharedApplication];
	[application setIdleTimerDisabled:!allowsScreenSaver];
}
