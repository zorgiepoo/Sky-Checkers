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

#import "defaults.h"
#import <Foundation/Foundation.h>

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

void getDefaultUserName(char *defaultUserName, int maxLength)
{
	if (maxLength > 0)
	{
		defaultUserName[0] = '\0';
	}
}
