/*
 * Copyright 2010 Mayur Pawashe
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

#import "osx.h"
#import <Cocoa/Cocoa.h>

FILE *getUserDataFile(const char *mode)
{
	@autoreleasepool
	{
		FILE *file = NULL;
		NSArray *userLibraryPaths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
		if (userLibraryPaths.count > 0)
		{
			NSString *userLibraryPath = [userLibraryPaths objectAtIndex:0];
			NSString *appSupportPath = [userLibraryPath stringByAppendingPathComponent:@"Application Support"];
			
			if ([[NSFileManager defaultManager] fileExistsAtPath:appSupportPath])
			{
				NSString *skyCheckersPath = [appSupportPath stringByAppendingPathComponent:@"SkyCheckers"];
				
				if ([[NSFileManager defaultManager] fileExistsAtPath:skyCheckersPath] || [[NSFileManager defaultManager] createDirectoryAtPath:skyCheckersPath withIntermediateDirectories:NO attributes:nil error:NULL])
				{
					file = fopen([[skyCheckersPath stringByAppendingPathComponent:@"user_data.txt"] UTF8String], mode);
				}
			}
		}
		
		return file;
	}
}

void getDefaultUserName(char *defaultUserName, int maxLength)
{
	@autoreleasepool
	{
		NSString *fullUsername = NSFullUserName();
		if (fullUsername)
		{
			NSUInteger spaceIndex = [fullUsername rangeOfString:@" "].location;
			if (spaceIndex != NSNotFound)
			{
				strncpy(defaultUserName, [[fullUsername substringToIndex:spaceIndex] UTF8String], maxLength);
			}
			else
			{
				strncpy(defaultUserName, [fullUsername UTF8String], maxLength);
			}
		}
	}
}

@interface ZGFullscreenHandler : NSObject
{
@public
	Renderer *_renderer;
}
@end

@implementation ZGFullscreenHandler

- (void)windowWillEnterFullScreen:(NSNotification *)notification
{
	_renderer->macosInFullscreenTransition = true;
}

- (void)windowDidEnterFullScreen:(NSNotification *)notification
{
	_renderer->macosInFullscreenTransition = false;
	_renderer->fullscreen = true;
}

- (void)windowWillExitFullScreen:(NSNotification *)notification
{
	_renderer->macosInFullscreenTransition = true;
}

- (void)windowDidExitFullScreen:(NSNotification *)notification
{
	_renderer->macosInFullscreenTransition = false;
	_renderer->fullscreen = false;
}

@end

static ZGFullscreenHandler *gFullscreenHandler;

void registerForNativeFullscreenEvents(void *windowReference, Renderer *renderer, bool toggleFullscreen)
{
	NSWindow *window = (__bridge NSWindow *)windowReference;
	
	assert(gFullscreenHandler == nil);
	gFullscreenHandler = [[ZGFullscreenHandler alloc] init];
	gFullscreenHandler->_renderer = renderer;
	
	// Register for full screen notifications
	NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
	
	[notificationCenter addObserver:gFullscreenHandler selector:@selector(windowDidEnterFullScreen:) name:NSWindowDidEnterFullScreenNotification object:window];
	
	[notificationCenter addObserver:gFullscreenHandler selector:@selector(windowDidExitFullScreen:) name:NSWindowDidExitFullScreenNotification object:window];
	
	[notificationCenter addObserver:gFullscreenHandler selector:@selector(windowWillEnterFullScreen:) name:NSWindowWillEnterFullScreenNotification object:window];
	
	if (toggleFullscreen)
	{
		[window toggleFullScreen:nil];
	}
}

void setVsyncForGL(bool enabled)
{
	NSOpenGLContext *context = [NSOpenGLContext currentContext];
	GLint value = !!enabled;
	
	[context setValues:&value forParameter:NSOpenGLCPSwapInterval];
}
