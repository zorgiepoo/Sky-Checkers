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
#import <UIKit/UIKit.h>

@interface ZGViewController : UIViewController
@end

@implementation ZGViewController
{
	CGRect _frame;
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

- (BOOL)prefersStatusBarHidden
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

void ZGSetWindowEventHandler(ZGWindow *window, void *context, void (*windowEventHandler)(ZGWindowEvent, void *))
{
}

void ZGPollWindowAndInputEvents(ZGWindow *window, const void *systemEvent)
{
}

void *ZGWindowHandle(ZGWindow *window)
{
	return window;
}
