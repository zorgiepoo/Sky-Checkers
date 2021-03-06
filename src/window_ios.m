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
#include "zgtime.h"
#import <UIKit/UIKit.h>

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

ZGWindow *ZGCreateWindow(const char *windowTitle, int32_t windowWidth, int32_t windowHeight, bool *fullscreenFlag)
{
	CGRect screenBounds = [[UIScreen mainScreen] bounds];
	UIWindow *window = [[UIWindow alloc] initWithFrame:screenBounds];
	window.rootViewController = [[ZGViewController alloc] initWithFrame:screenBounds];
	[window makeKeyAndVisible];
	
	return (ZGWindow *)CFBridgingRetain(window);
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
