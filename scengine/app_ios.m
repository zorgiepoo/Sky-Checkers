/*
 MIT License

 Copyright (c) 2019 Mayur Pawashe

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

#import "app.h"
#import "app_delegate_ios.h"
#import "zgtime.h"

#import <UIKit/UIKit.h>

typedef struct
{
	ZGAppHandlers *appHandlers;
	void *context;
} ZGAppCallbacks;

static ZGAppCallbacks gAppCallbacks;

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
	
	// Initialize time
	(void)ZGGetNanoTicks();
	
	return YES;
}

// Later called by ZGGameSceneDelegate
- (void)applicationDidShowMainWindowScene
{
	if (gAppCallbacks.appHandlers->launchedHandler != NULL)
	{
		gAppCallbacks.appHandlers->launchedHandler(gAppCallbacks.context);
	}
	
	if (gAppCallbacks.appHandlers->runLoopHandler != NULL)
	{
		_displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(runLoop:)];
		[_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
	}
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

void ZGAppSetAllowsScreenIdling(bool allowsScreenIdling)
{
	UIApplication *application = [UIApplication sharedApplication];
	[application setIdleTimerDisabled:!allowsScreenIdling];
}
