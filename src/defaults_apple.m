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

#import "defaults.h"
#import <Foundation/Foundation.h>

#if PLATFORM_OSX

FILE *getUserDataFile(const char *mode)
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

#elif PLATFORM_IOS

FILE *getUserDataFile(const char *mode)
{
	FILE *file = NULL;
	NSArray *userDocumentPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	if (userDocumentPaths.count > 0)
	{
		NSString *userDocumentPath = [userDocumentPaths objectAtIndex:0];
		NSString *skyCheckersPath = [userDocumentPath stringByAppendingPathComponent:@"SkyCheckers"];
		
		if ([[NSFileManager defaultManager] fileExistsAtPath:skyCheckersPath] || [[NSFileManager defaultManager] createDirectoryAtPath:skyCheckersPath withIntermediateDirectories:NO attributes:nil error:NULL])
		{
			file = fopen([[skyCheckersPath stringByAppendingPathComponent:@"user_data.txt"] UTF8String], mode);
		}
	}
	
	return file;
}

#endif
