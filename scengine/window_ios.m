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

#import "window.h"
#import "app_delegate_ios.h"
#include "zgtime.h"

#if PLATFORM_TVOS
#define ZGMenuTapRecognizerName @"ZGMenuTapRecognizerName"
#else
#define ZGTapRecognizerName @"ZGTapRecognizerName"
#define ZGPannedRecognizerName @"ZGPannedRecognizerName"
#endif

@interface ZGViewController : UIViewController <UIGestureRecognizerDelegate>

@property (nonatomic) void (*touchEventHandler)(ZGTouchEvent, void *);
@property (nonatomic) void *touchEventContext;

@end

@implementation ZGViewController
{
	CGRect _frame;
#if PLATFORM_TVOS
	UITapGestureRecognizer *_menuTapGestureRecognizer;
#else
	UIPanGestureRecognizer *_panGestureRecognizer;
	UITapGestureRecognizer *_tapGestureRecognizer;
#endif
}

- (instancetype)initWithFrame:(CGRect)frame
{
	self = [super init];
	if (self != nil)
	{
		_frame = frame;
	}
	return self;
}

- (void)loadView
{
	UIView *view = [[UIView alloc] initWithFrame:_frame];
	view.backgroundColor = UIColor.blackColor;
	
	self.view = view;
}

#if PLATFORM_TVOS
- (void)installMenuGesture
{
	if (_menuTapGestureRecognizer == nil)
	{
		UITapGestureRecognizer *tapGestureRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(menuGestureTapped:)];
		
		tapGestureRecognizer.name = ZGMenuTapRecognizerName;
		tapGestureRecognizer.allowedPressTypes = @[@(UIPressTypeMenu)];
		_menuTapGestureRecognizer = tapGestureRecognizer;
		
		[self.view addGestureRecognizer:tapGestureRecognizer];
	}
}

- (void)uninstallMenuGesture
{
	if (_menuTapGestureRecognizer != nil)
	{
		[self.view removeGestureRecognizer:_menuTapGestureRecognizer];
		_menuTapGestureRecognizer = nil;
	}
}
#else
- (void)installTouchGestures
{
	UIView *view = self.view;
	
	if (_panGestureRecognizer == nil)
	{
		UIPanGestureRecognizer *panGestureRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(viewGesturePanned:)];
		
		panGestureRecognizer.name = ZGPannedRecognizerName;
		panGestureRecognizer.maximumNumberOfTouches = 1;
		panGestureRecognizer.delegate = self;
		_panGestureRecognizer = panGestureRecognizer;
		
		[view addGestureRecognizer:panGestureRecognizer];
	}
	
	if (_tapGestureRecognizer == nil)
	{
		UITapGestureRecognizer *tapGestureRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(viewGestureTapped:)];
		
		tapGestureRecognizer.name = ZGTapRecognizerName;
		tapGestureRecognizer.delegate = self;
		_tapGestureRecognizer = tapGestureRecognizer;
		
		[view addGestureRecognizer:tapGestureRecognizer];
	}
}

- (void)uninstallTouchGestures
{
	UIView *view = self.view;
	
	if (_panGestureRecognizer != nil)
	{
		[view removeGestureRecognizer:_panGestureRecognizer];
		_panGestureRecognizer = nil;
	}
	
	if (_tapGestureRecognizer != nil)
	{
		[view removeGestureRecognizer:_tapGestureRecognizer];
		_tapGestureRecognizer = nil;
	}
}
#endif

#if PLATFORM_TVOS
- (void)menuGestureTapped:(UITapGestureRecognizer *)tapGestureRecognizer
{
	if (tapGestureRecognizer.state == UIGestureRecognizerStateEnded)
	{
		if (_touchEventHandler != NULL)
		{
			ZGTouchEvent event = {};
			event.type = ZGTouchEventTypeMenuTap;
			_touchEventHandler(event, _touchEventContext);
		}
	}
}
#else
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer
{
	NSArray<NSString *> *gestureNames = @[ZGTapRecognizerName, ZGPannedRecognizerName];
	return [gestureNames containsObject:gestureRecognizer.name] && [gestureNames containsObject:otherGestureRecognizer.name];
}

- (void)viewGesturePanned:(UIPanGestureRecognizer *)gestureRecognizer
{
	switch (gestureRecognizer.state)
	{
		case UIGestureRecognizerStateBegan:
		case UIGestureRecognizerStateChanged:
		{
			if (_touchEventHandler != NULL)
			{
				CGPoint touchPoint = [gestureRecognizer locationInView:self.view];
				CGPoint velocity = [gestureRecognizer velocityInView:self.view];
				
				ZGTouchEvent event = {};
				event.deltaX = (float)velocity.x;
				event.deltaY = (float)velocity.y;
				event.x = (float)touchPoint.x;
				event.y = (float)touchPoint.y;
				event.timestamp = ZGGetNanoTicks();
				event.type = ZGTouchEventTypePanChanged;
				
				_touchEventHandler(event, _touchEventContext);
			}
		} break;
		case UIGestureRecognizerStateFailed:
		case UIGestureRecognizerStateCancelled:
		case UIGestureRecognizerStateEnded:
			if (_touchEventHandler != NULL)
			{
				ZGTouchEvent event = {};
				event.type = ZGTouchEventTypePanEnded;
				
				_touchEventHandler(event, _touchEventContext);
			}
			break;
		case UIGestureRecognizerStatePossible:
			break;
	}
}

- (void)viewGestureTapped:(UITapGestureRecognizer *)tapGestureRecognizer
{
	if (tapGestureRecognizer.state == UIGestureRecognizerStateEnded)
	{
		if (_touchEventHandler != NULL)
		{
			CGPoint touchPoint = [tapGestureRecognizer locationInView:self.view];
			
			ZGTouchEvent event = {};
			event.type = ZGTouchEventTypeTap;
			event.x = (float)touchPoint.x;
			event.y = (float)touchPoint.y;
			_touchEventHandler(event, _touchEventContext);
		}
	}
}

- (BOOL)prefersStatusBarHidden
{
	return YES;
}
#endif

- (BOOL)prefersHomeIndicatorAutoHidden
{
	return YES;
}

@end

@interface ZGGameSceneDelegate : UIResponder <UIWindowSceneDelegate>

@property (nonatomic, nullable) UIWindow *window;

@property (nonatomic, nullable) void (*windowEventHandler)(ZGWindowEvent, void *);
@property (nonatomic, nullable) void *windowEventHandlerContext;

@end

@implementation ZGGameSceneDelegate

- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions
{
	if (![scene isKindOfClass:UIWindowScene.class])
	{
		NSLog(@"Error: Encountered unexpected scene type to connect: %@", scene);
		return;
	}
	
	if (![session.role isEqualToString:UIWindowSceneSessionRoleApplication])
	{
		return;
	}
	
	UIWindowScene *windowScene = (UIWindowScene *)scene;
	
	// We just have some defensive checks here if the _window already exists
	// I doubt a new window scene will be connected but just in case try to handle that
	if (_window == nil)
	{
		// Handle first window session connection
		UIWindow *window = [[UIWindow alloc] initWithWindowScene:windowScene];
		window.rootViewController = [[ZGViewController alloc] initWithFrame:window.bounds];
		
		_window = window;
		
		[window makeKeyAndVisible];
		
		// Window is ready for the app to begin launching and using it now
		ZGAppDelegate *appDelegate = (ZGAppDelegate * _Nonnull)UIApplication.sharedApplication.delegate;
		assert([appDelegate isKindOfClass:ZGAppDelegate.class]);
		
		[appDelegate applicationDidShowMainWindowScene];
	}
	else if (_window.windowScene == windowScene)
	{
		[_window makeKeyAndVisible];
	}
	else
	{
		_window.windowScene = windowScene;
		[_window makeKeyAndVisible];
	}
}

- (void)sceneDidBecomeActive:(UIScene *)scene
{
	if (_windowEventHandler != nil)
	{
		ZGWindowEvent windowEvent = { 0 };
		windowEvent.type = ZGWindowEventTypeFocusGained;
		_windowEventHandler(windowEvent, _windowEventHandlerContext);
	}
}

- (void)sceneWillResignActive:(UIScene *)scene
{
	if (_windowEventHandler != nil)
	{
		ZGWindowEvent windowEvent = { 0 };
		windowEvent.type = ZGWindowEventTypeFocusLost;
		_windowEventHandler(windowEvent, _windowEventHandlerContext);
	}
}

- (void)sceneWillEnterForeground:(UIScene *)scene
{
	if (_windowEventHandler != nil)
	{
		ZGWindowEvent windowEvent = { 0 };
		windowEvent.type = ZGWindowEventTypeShown;
		_windowEventHandler(windowEvent, _windowEventHandlerContext);
	}
}

- (void)sceneDidEnterBackground:(UIScene *)scene
{
	if (_windowEventHandler != nil)
	{
		ZGWindowEvent windowEvent = { 0 };
		windowEvent.type = ZGWindowEventTypeHidden;
		_windowEventHandler(windowEvent, _windowEventHandlerContext);
	}
}

#if !PLATFORM_TVOS
- (void)windowScene:(UIWindowScene *)windowScene didUpdateCoordinateSpace:(id<UICoordinateSpace>)previousCoordinateSpace interfaceOrientation:(UIInterfaceOrientation)previousInterfaceOrientation traitCollection:(UITraitCollection *)previousTraitCollection
{
	if (windowScene.activationState == UISceneActivationStateUnattached)
	{
		return;
	}
	
	if (_windowEventHandler == nil)
	{
		return;
	}
	
	UIWindow *currentWindow = self.window;
	for (UIWindow *sceneWindow in windowScene.windows)
	{
		if (sceneWindow == currentWindow)
		{
			CGRect frame = currentWindow.frame;
			
			ZGWindowEvent windowEvent = { 0 };
			windowEvent.width = (int32_t)frame.size.width;
			windowEvent.height = (int32_t)frame.size.height;
			windowEvent.type = ZGWindowEventTypeResize;
			
			_windowEventHandler(windowEvent, _windowEventHandlerContext);
			
			break;
		}
	}
}
#endif

@end

ZGWindow *ZGCreateWindow(const char *windowTitle, int32_t windowWidth, int32_t windowHeight, bool *fullscreenFlag)
{
	for (UIScene *scene in UIApplication.sharedApplication.connectedScenes)
	{
		ZGGameSceneDelegate *sceneDelegate = (ZGGameSceneDelegate *)scene.delegate;
		if (![sceneDelegate isKindOfClass:ZGGameSceneDelegate.class])
		{
			continue;
		}
		
		assert([scene isKindOfClass:UIWindowScene.class]);
		
		UIWindow *window = sceneDelegate.window;
		assert(window != nil);
		
		return (ZGWindow *)CFBridgingRetain(window);
	}
	
	NSLog(@"Error: failed to find key window from connected scene!");
	return nil;
}

void ZGDestroyWindow(ZGWindow *window)
{
	CFRelease(window);
}

bool ZGWindowHasFocus(ZGWindow *windowRef)
{
	UIWindow *window = (__bridge UIWindow *)(windowRef);
	return [window isKeyWindow];
}

void ZGSetWindowEventHandler(ZGWindow *window, void *context, void (*windowEventHandler)(ZGWindowEvent, void *))
{
	for (UIScene *scene in UIApplication.sharedApplication.connectedScenes)
	{
		ZGGameSceneDelegate *sceneDelegate = (ZGGameSceneDelegate *)scene.delegate;
		if (![sceneDelegate isKindOfClass:ZGGameSceneDelegate.class])
		{
			continue;
		}
		
		assert([scene isKindOfClass:UIWindowScene.class]);
		
		UIWindowScene *windowScene = (UIWindowScene *)scene;
		
		for (UIWindow *uiWindow in windowScene.windows)
		{
			if ((__bridge UIWindow *)window != uiWindow)
			{
				continue;
			}
			
			sceneDelegate.windowEventHandler = windowEventHandler;
			sceneDelegate.windowEventHandlerContext = context;
			
			return;
		}
	}
}

void ZGSetTouchEventHandler(ZGWindow *windowRef, void *context, void (*touchEventHandler)(ZGTouchEvent, void *))
{
	UIWindow *window = (__bridge UIWindow *)(windowRef);
	ZGViewController *viewController = (ZGViewController *)window.rootViewController;
	assert([viewController isKindOfClass:[ZGViewController class]]);
	viewController.touchEventHandler = touchEventHandler;
	viewController.touchEventContext = context;
}

#if PLATFORM_TVOS
void ZGInstallMenuGesture(ZGWindow *windowRef)
{
	UIWindow *window = (__bridge UIWindow *)(windowRef);
	ZGViewController *viewController = (ZGViewController *)window.rootViewController;
	assert([viewController isKindOfClass:[ZGViewController class]]);
	
	[viewController installMenuGesture];
}

void ZGUninstallMenuGesture(ZGWindow *windowRef)
{
	UIWindow *window = (__bridge UIWindow *)(windowRef);
	ZGViewController *viewController = (ZGViewController *)window.rootViewController;
	assert([viewController isKindOfClass:[ZGViewController class]]);
	
	[viewController uninstallMenuGesture];
}

#else
void ZGInstallTouchGestures(ZGWindow *windowRef)
{
	UIWindow *window = (__bridge UIWindow *)(windowRef);
	ZGViewController *viewController = (ZGViewController *)window.rootViewController;
	assert([viewController isKindOfClass:[ZGViewController class]]);
	
	[viewController installTouchGestures];
}

void ZGUninstallTouchGestures(ZGWindow *windowRef)
{
	UIWindow *window = (__bridge UIWindow *)(windowRef);
	ZGViewController *viewController = (ZGViewController *)window.rootViewController;
	assert([viewController isKindOfClass:[ZGViewController class]]);
	
	[viewController uninstallTouchGestures];
}
#endif

void ZGGetWindowSize(ZGWindow *windowRef, int32_t *width, int32_t *height)
{
	UIWindow *window = (__bridge UIWindow *)(windowRef);
	
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
	return window;
}
